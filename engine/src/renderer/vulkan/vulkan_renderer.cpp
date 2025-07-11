#include "renderer/vulkan/vulkan_renderer.h"

#include <renderer/vulkan/pass/lighting_pass.h>

#include "renderer/vulkan/vulkan_descriptor_writer.h"
#include "renderer/vulkan/vulkan_framebuffer.h"
#include "renderer/vulkan/vulkan_device.h"
#include "renderer/vulkan/vulkan_mesh.h"
#include "renderer/vulkan/vulkan_shader_compiler.h"
#include "renderer/vulkan/lighting/irradiance_map_generator.h"
#include "renderer/vulkan/lighting/reflection_probe_generator.h"

#include "imgui.h"

#include "resource/resource_manager.h"

#include "util/log.h"
#include "util/filesystem.h"

namespace MongooseVK
{
    VulkanRenderer::~VulkanRenderer()
    {
        device->DestroyBuffer(descriptorBuffers.cameraBuffer);
        device->DestroyBuffer(descriptorBuffers.lightsBuffer);
    }

    void VulkanRenderer::Init(const uint32_t width, const uint32_t height)
    {
        device = VulkanDevice::Get();

        LOG_TRACE("VulkanRenderer::Init()");
        viewportResolution.width = width;
        viewportResolution.height = height;

        renderResolution.width = resolutionScale * viewportResolution.width;
        renderResolution.height = resolutionScale * viewportResolution.height;

        shaderCache = CreateScope<ShaderCache>(device);

        CreateSwapchain();
    }

    void VulkanRenderer::LoadScene(const std::string& gltfPath, const std::string& hdrPath)
    {
        isSceneLoaded = false;

        LOG_TRACE("Load scene");
        scene = ResourceManager::LoadScene(device, gltfPath, hdrPath);
        scene.directionalLight.direction = normalize(glm::vec3(0.0f, -2.0f, -1.0f));

        BuildGBuffer();

        renderPasses.gbufferPass = CreateScope<GBufferPass>(device, scene, renderResolution);
        renderPasses.skyboxPass = CreateScope<SkyboxPass>(device, scene, renderResolution);
        renderPasses.lightingPass = CreateScope<LightingPass>(device, scene, renderResolution);
        renderPasses.shadowMapPass = CreateScope<ShadowMapPass>(device, scene, renderResolution);
        renderPasses.presentPass = CreateScope<PresentPass>(device, renderResolution);
        renderPasses.ssaoPass = CreateScope<SSAOPass>(device, renderResolution);
        renderPasses.gridPass = CreateScope<InfiniteGridPass>(device, renderResolution);

        CreateShadowMap();
        CreateFramebuffers();
        CreateCameraBuffer();
        CreateLightsBuffer();
        CreatePresentDescriptorSet();

        PrepareSSAO();
        PrecomputeIBL();

        CreateRenderPassResources();

        isSceneLoaded = true;
    }

    void VulkanRenderer::PrecomputeIBL()
    {
        irradianceMap = IrradianceMapGenerator(device).FromCubemapTexture(scene.skybox->GetCubemap());
        const VkDescriptorImageInfo irradianceMapInfo = {
            .sampler = irradianceMap->GetSampler(),
            .imageView = irradianceMap->GetImageView(),
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };

        VulkanDescriptorWriter(*shaderCache->descriptorSetLayouts.irradianceDescriptorSetLayout, device->GetShaderDescriptorPool())
                .WriteImage(0, &irradianceMapInfo)
                .Build(shaderCache->descriptorSets.irradianceDescriptorSet);

        scene.reflectionProbe = ReflectionProbeGenerator(device).FromCubemap(scene.skybox->GetCubemap());
    }

    void VulkanRenderer::Draw(const float deltaTime, Camera& camera)
    {
        if (!isSceneLoaded) return;
        device->DrawFrame(vulkanSwapChain->GetSwapChain(),
                          [&](const VkCommandBuffer cmd, const uint32_t imgIndex) {
                              RotateLight(deltaTime);
                              scene.directionalLight.UpdateCascades(camera);

                              UpdateLightsBuffer();
                              UpdateCameraBuffer(camera);
                              DrawFrame(cmd, imgIndex, camera);
                          },
                          std::bind(&VulkanRenderer::ResizeSwapchain, this));
    }

    void VulkanRenderer::IdleWait()
    {
        vkDeviceWaitIdle(device->GetDevice());
    }

    void VulkanRenderer::Resize(const int width, const int height)
    {
        viewportResolution.width = width;
        viewportResolution.height = height;

        renderResolution.width = resolutionScale * viewportResolution.width;
        renderResolution.height = resolutionScale * viewportResolution.height;

        IdleWait();
        device->GetReadyToResize();
    }

    void VulkanRenderer::CreateSwapchain()
    {
        LOG_TRACE("Vulkan: create swapchain");
        vulkanSwapChain = nullptr;

        const auto swapChainSupport = VulkanUtils::QuerySwapChainSupport(device->GetPhysicalDevice(), device->GetSurface());
        const VkSurfaceFormatKHR surfaceFormat = VulkanUtils::ChooseSwapSurfaceFormat(swapChainSupport.formats);

        const uint32_t imageCount = VulkanUtils::GetSwapchainImageCount(device->GetPhysicalDevice(), device->GetSurface());
        vulkanSwapChain = VulkanSwapchain::Builder(device)
                .SetResolution(viewportResolution.width, viewportResolution.height)
                .SetPresentMode(VK_PRESENT_MODE_IMMEDIATE_KHR)
                .SetImageCount(imageCount)
                .SetImageFormat(surfaceFormat.format)
                .Build();
    }

    void VulkanRenderer::CreatePresentDescriptorSet()
    {
        const VkDescriptorImageInfo renderImageInfo = {
            .sampler = framebuffers.geometryFramebuffer->GetAttachments()[0].sampler,
            .imageView = framebuffers.geometryFramebuffer->GetAttachments()[0].imageView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };

        VulkanDescriptorWriter(*ShaderCache::descriptorSetLayouts.presentDescriptorSetLayout, device->GetShaderDescriptorPool())
                .WriteImage(0, &renderImageInfo)
                .BuildOrOverwrite(shaderCache->descriptorSets.presentDescriptorSet);
    }

    void VulkanRenderer::CreateShadowMap()
    {
        const uint16_t SHADOW_MAP_RESOLUTION = EnumValue(scene.directionalLight.shadowMapResolution);
        directionalShadowMap = VulkanShadowMap::Builder()
                .SetResolution(SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION)
                .SetArrayLayers(SHADOW_MAP_CASCADE_COUNT)
                .Build(device);

        framebuffers.shadowMapFramebuffers.resize(SHADOW_MAP_CASCADE_COUNT);
        for (size_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
        {
            framebuffers.shadowMapFramebuffers[i] = VulkanFramebuffer::Builder(device)
                    .SetRenderpass(renderPasses.shadowMapPass->GetRenderPass())
                    .SetResolution(SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION)
                    .AddAttachment(directionalShadowMap->GetImageView(i))
                    .Build();
        }
    }

    void VulkanRenderer::CreateFramebuffers()
    {
        framebuffers.ssaoFramebuffer = VulkanFramebuffer::Builder(device)
                .SetRenderpass(renderPasses.ssaoPass->GetRenderPass())
                .SetResolution(viewportResolution.width * 0.5, viewportResolution.height * 0.5)
                .AddAttachment(ImageFormat::R8_UNORM)
                .Build();

        framebuffers.geometryFramebuffer = VulkanFramebuffer::Builder(device)
                .SetRenderpass(renderPasses.lightingPass->GetRenderPass())
                .SetResolution(renderResolution.width, renderResolution.height)
                .AddAttachment(ImageFormat::RGBA16_SFLOAT)
                .AddAttachment(ImageFormat::DEPTH24_STENCIL8)
                .Build();

        framebuffers.gbufferFramebuffer = VulkanFramebuffer::Builder(device)
                .SetRenderpass(renderPasses.gbufferPass->GetRenderPass())
                .SetResolution(viewportResolution.width, viewportResolution.height)
                .AddAttachment(gBuffer->buffers.viewSpaceNormal.imageView)
                .AddAttachment(gBuffer->buffers.viewSpacePosition.imageView)
                .AddAttachment(gBuffer->buffers.depth.imageView)
                .Build();

        const uint32_t imageCount = VulkanUtils::GetSwapchainImageCount(device->GetPhysicalDevice(), device->GetSurface());
        framebuffers.presentFramebuffers.clear();
        framebuffers.presentFramebuffers.resize(imageCount);
        for (size_t i = 0; i < imageCount; i++)
        {
            framebuffers.presentFramebuffers[i] = VulkanFramebuffer::Builder(device)
                    .SetRenderpass(renderPasses.presentPass->GetRenderPass())
                    .SetResolution(viewportResolution.width, viewportResolution.height)
                    .AddAttachment(vulkanSwapChain->GetImageViews()[i])
                    .Build();
        }
    }

    void VulkanRenderer::ResizeSwapchain()
    {
        IdleWait();
        CreateSwapchain();

        renderPasses.gbufferPass->Resize(renderResolution);
        renderPasses.skyboxPass->Resize(renderResolution);
        renderPasses.lightingPass->Resize(renderResolution);
        renderPasses.shadowMapPass->Resize(renderResolution);
        renderPasses.ssaoPass->Resize(renderResolution);
        renderPasses.gridPass->Resize(renderResolution);
        renderPasses.presentPass->Resize(viewportResolution);

        BuildGBuffer();
        CreateFramebuffers();
        CreatePresentDescriptorSet();

        PrepareSSAO();
        CreateRenderPassResources();
    }

    void VulkanRenderer::CreateCameraBuffer()
    {
        descriptorBuffers.cameraBuffer = device->CreateBuffer(sizeof(TransformsBuffer),
                                                              VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                                                              VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                                              VMA_MEMORY_USAGE_CPU_TO_GPU);

        VkDescriptorBufferInfo info{};
        info.buffer = descriptorBuffers.cameraBuffer.buffer;
        info.offset = 0;
        info.range = sizeof(TransformsBuffer);

        VulkanDescriptorWriter(*shaderCache->descriptorSetLayouts.cameraDescriptorSetLayout, device->GetShaderDescriptorPool())
                .WriteBuffer(0, &info)
                .Build(shaderCache->descriptorSets.cameraDescriptorSet);
    }

    void VulkanRenderer::CreateLightsBuffer()
    {
        descriptorBuffers.lightsBuffer = device->CreateBuffer(sizeof(LightsBuffer),
                                                              VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                                                              VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                                              VMA_MEMORY_USAGE_CPU_TO_GPU);

        VkDescriptorBufferInfo info{};
        info.buffer = descriptorBuffers.lightsBuffer.buffer;
        info.offset = 0;
        info.range = sizeof(LightsBuffer);

        VkDescriptorImageInfo shadowMapInfo{};
        shadowMapInfo.sampler = directionalShadowMap->GetSampler();
        shadowMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        shadowMapInfo.imageView = directionalShadowMap->GetImageView();

        VulkanDescriptorWriter(*shaderCache->descriptorSetLayouts.lightsDescriptorSetLayout, device->GetShaderDescriptorPool())
                .WriteBuffer(0, &info)
                .Build(shaderCache->descriptorSets.lightsDescriptorSet);

        VulkanDescriptorWriter(*shaderCache->descriptorSetLayouts.directionalShadowMapDescriptorSetLayout,
                               device->GetShaderDescriptorPool())
                .WriteImage(0, &shadowMapInfo)
                .Build(shaderCache->descriptorSets.directionalShadownMapDescriptorSet);
    }

    void VulkanRenderer::UpdateCameraBuffer(Camera& camera) const
    {
        TransformsBuffer bufferData;
        bufferData.cameraPosition = camera.GetTransform().m_Position;
        bufferData.view = camera.GetView();
        bufferData.proj = camera.GetProjection();

        memcpy(descriptorBuffers.cameraBuffer.GetData(), &bufferData, sizeof(TransformsBuffer));
    }

    void VulkanRenderer::RotateLight(float deltaTime)
    {
        Transform lightTransform;
        lightTransform.m_Rotation = glm::vec3(45.0f, 0.0f, 0.0f);

        const glm::mat4 rot_mat = rotate(glm::mat4(1.0f), glm::radians(22.5f * deltaTime), glm::vec3(0.0, 1.0, 0.0));
        scene.directionalLight.direction = normalize(glm::vec3(glm::vec4(scene.directionalLight.direction, 1.0f) * rot_mat));
    }

    void VulkanRenderer::UpdateLightsBuffer()
    {
        LightsBuffer bufferData;

        for (size_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
        {
            bufferData.lightProjection[i] = scene.directionalLight.cascades[i].viewProjMatrix;
            bufferData.cascadeSplits[i] = scene.directionalLight.cascades[i].splitDepth;
        }

        bufferData.direction = glm::vec4(normalize(scene.directionalLight.direction), 0.0);
        bufferData.color = glm::vec4(scene.directionalLight.color, 1.0f);
        bufferData.intensity = scene.directionalLight.intensity;
        bufferData.ambientIntensity = scene.directionalLight.ambientIntensity;
        bufferData.bias = scene.directionalLight.bias;

        memcpy(descriptorBuffers.lightsBuffer.GetData(), &bufferData, sizeof(LightsBuffer));
    }

    void VulkanRenderer::DrawFrame(const VkCommandBuffer commandBuffer, const uint32_t imageIndex, Camera& camera)
    {
        activeImage = imageIndex;

        renderPasses.gbufferPass->Render(commandBuffer, camera, framebuffers.gbufferFramebuffer);
        // TODO Don't forget to add image barriers back later
        //directionalShadowMap->TransitionToDepthRendering(commandBuffer);
        for (size_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
        {
            renderPasses.shadowMapPass->SetCascadeIndex(i);
            renderPasses.shadowMapPass->Render(commandBuffer, camera, framebuffers.shadowMapFramebuffers[i]);
        }
        //directionalShadowMap->TransitionToShaderRead(commandBuffer);

        renderPasses.ssaoPass->Render(commandBuffer, camera, framebuffers.ssaoFramebuffer);
        renderPasses.skyboxPass->Render(commandBuffer, camera, framebuffers.geometryFramebuffer);

        renderPasses.lightingPass->Render(commandBuffer, camera, framebuffers.geometryFramebuffer);
        renderPasses.gridPass->Render(commandBuffer, camera, framebuffers.geometryFramebuffer);

        renderPasses.presentPass->Render(commandBuffer, camera, framebuffers.presentFramebuffers[activeImage]);
    }

    void VulkanRenderer::BuildGBuffer()
    {
        gBuffer = VulkanGBuffer::Builder()
                .SetResolution(viewportResolution.width, viewportResolution.height)
                .Build(device);

        const VkDescriptorImageInfo worldSpaceNormalInfo{
            .sampler = gBuffer->buffers.viewSpaceNormal.sampler,
            .imageView = gBuffer->buffers.viewSpaceNormal.imageView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };

        const VkDescriptorImageInfo positionInfo{
            .sampler = gBuffer->buffers.viewSpacePosition.sampler,
            .imageView = gBuffer->buffers.viewSpacePosition.imageView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };

        const VkDescriptorImageInfo depthInfo{
            .sampler = gBuffer->buffers.depth.sampler,
            .imageView = gBuffer->buffers.depth.imageView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };

        VulkanDescriptorWriter(*ShaderCache::descriptorSetLayouts.viewspaceNormalDescriptorSetLayout, device->GetShaderDescriptorPool())
                .WriteImage(0, &worldSpaceNormalInfo)
                .BuildOrOverwrite(ShaderCache::descriptorSets.viewspaceNormalDescriptorSet);

        VulkanDescriptorWriter(*ShaderCache::descriptorSetLayouts.viewspacePositionDescriptorSetLayout, device->GetShaderDescriptorPool())
                .WriteImage(0, &positionInfo)
                .BuildOrOverwrite(ShaderCache::descriptorSets.viewspacePositionDescriptorSet);

        VulkanDescriptorWriter(*ShaderCache::descriptorSetLayouts.depthMapDescriptorSetLayout, device->GetShaderDescriptorPool())
                .WriteImage(0, &depthInfo)
                .BuildOrOverwrite(ShaderCache::descriptorSets.depthMapDescriptorSet);
    }

    void VulkanRenderer::PrepareSSAO()
    {
        const VkDescriptorImageInfo ssaoInfo = {
            .sampler = framebuffers.ssaoFramebuffer->GetAttachments()[0].sampler,
            .imageView = framebuffers.ssaoFramebuffer->GetAttachments()[0].imageView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };

        VulkanDescriptorWriter(*ShaderCache::descriptorSetLayouts.postProcessingDescriptorSetLayout, device->GetShaderDescriptorPool())
                .WriteImage(0, &ssaoInfo)
                .BuildOrOverwrite(ShaderCache::descriptorSets.postProcessingDescriptorSet);
    }

    void VulkanRenderer::CreateRenderPassResources()
    {
        // Lights Buffer
        {
            ResourceBufferInfo lightsBufferInfo = {
                .size = sizeof(LightsBuffer)
            };
            renderPassResources.lightsBuffer = {
                .name = "lights_buffer",
                .type = ResourceType::Buffer,
                .descriptorSet = shaderCache->descriptorSets.lightsDescriptorSet,
                .descriptorSetLayout = shaderCache->descriptorSetLayouts.lightsDescriptorSetLayout,
                .bufferInfo = lightsBufferInfo
            };
        }

        // Camera Buffer
        {
            renderPassResources.cameraBuffer = {
                .name = "camera_buffer",
                .type = ResourceType::Buffer,
                .descriptorSet = shaderCache->descriptorSets.cameraDescriptorSet,
                .descriptorSetLayout = shaderCache->descriptorSetLayouts.cameraDescriptorSetLayout
            };
        }

        // Viewspace Normal
        {
            ResourceTextureInfo textureInfo;
            textureInfo.textureCreateInfo = {
                .width = viewportResolution.width,
                .height = viewportResolution.height,
                .format = ImageFormat::RGBA32_SFLOAT,
                .imageLayout = ImageUtils::GetLayoutFromFormat(ImageFormat::RGBA32_SFLOAT),
                .addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            };

            renderPassResources.viewspaceNormal = {
                .name = "viewspace_normal",
                .type = ResourceType::Texture,
                .descriptorSet = shaderCache->descriptorSets.viewspaceNormalDescriptorSet,
                .descriptorSetLayout = shaderCache->descriptorSetLayouts.viewspaceNormalDescriptorSetLayout,
                .textureInfo = textureInfo,
            };
        }

        // Viewspace Position
        {
            ResourceTextureInfo textureInfo;
            textureInfo.textureCreateInfo = {
                .width = viewportResolution.width,
                .height = viewportResolution.height,
                .format = ImageFormat::RGBA32_SFLOAT,
                .imageLayout = ImageUtils::GetLayoutFromFormat(ImageFormat::RGBA32_SFLOAT),
                .addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            };

            renderPassResources.viewspacePosition = {
                .name = "viewspace_position",
                .type = ResourceType::Texture,
                .descriptorSet = shaderCache->descriptorSets.viewspacePositionDescriptorSet,
                .descriptorSetLayout = shaderCache->descriptorSetLayouts.viewspacePositionDescriptorSetLayout,
                .textureInfo = textureInfo,
            };
        }

        // Depth Map
        {
            ResourceTextureInfo textureInfo;
            textureInfo.textureCreateInfo = {
                .width = viewportResolution.width,
                .height = viewportResolution.height,
                .format = ImageFormat::DEPTH32,
                .imageLayout = ImageUtils::GetLayoutFromFormat(ImageFormat::DEPTH32),
                .addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            };

            renderPassResources.depthMap = {
                .name = "depth_map",
                .type = ResourceType::Texture,
                .descriptorSet = shaderCache->descriptorSets.depthMapDescriptorSet,
                .descriptorSetLayout = shaderCache->descriptorSetLayouts.depthMapDescriptorSetLayout,
                .textureInfo = textureInfo,
            };
        }

        // Directional Shadow Map
        {
            ResourceTextureInfo textureInfo;
            textureInfo.textureCreateInfo = {
                .width = viewportResolution.width,
                .height = viewportResolution.height,
                .format = ImageFormat::DEPTH32,
                .imageLayout = ImageUtils::GetLayoutFromFormat(ImageFormat::DEPTH32),
                .addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
                .arrayLayers = SHADOW_MAP_CASCADE_COUNT,
                .compareEnabled = true,
                .compareOp = VK_COMPARE_OP_LESS
            };

            renderPassResources.directionalShadowMap = {
                .name = "directional_shadow_map",
                .type = ResourceType::Texture,
                .descriptorSet = shaderCache->descriptorSets.directionalShadownMapDescriptorSet,
                .descriptorSetLayout = shaderCache->descriptorSetLayouts.directionalShadowMapDescriptorSetLayout,
                .textureInfo = textureInfo,
            };
        }

        // Irradiance Map
        {
            renderPassResources.irradianceMap = {
                .name = "irradiance_map",
                .type = ResourceType::Texture,
                .descriptorSet = shaderCache->descriptorSets.irradianceDescriptorSet,
                .descriptorSetLayout = shaderCache->descriptorSetLayouts.irradianceDescriptorSetLayout
            };
        }

        // SSAO Texture
        {
            renderPassResources.ssaoTexture = {
                .name = "ssao_texture",
                .type = ResourceType::Texture,
                .descriptorSet = shaderCache->descriptorSets.postProcessingDescriptorSet,
                .descriptorSetLayout = shaderCache->descriptorSetLayouts.postProcessingDescriptorSetLayout
            };
        }
    }
}
