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
    VulkanRenderer::~VulkanRenderer() {}

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

        gbufferPass = CreateScope<GBufferPass>(device, scene, renderResolution);
        skyboxPass = CreateScope<SkyboxPass>(device, scene, renderResolution);
        lightingPass = CreateScope<LightingPass>(device, scene, renderResolution);
        shadowMapPass = CreateScope<ShadowMapPass>(device, scene, renderResolution);
        presentPass = CreateScope<PresentPass>(device, renderResolution);
        ssaoPass = CreateScope<SSAOPass>(device, renderResolution);
        gridPass = CreateScope<InfiniteGridPass>(device, renderResolution);

        CreateShadowMap();
        CreateFramebuffers();
        CreateCameraBuffer();
        CreateLightsBuffer();
        CreatePresentDescriptorSet();

        PrepareSSAO();
        PrecomputeIBL();

        CreateRenderPassResources();

        gridPass->AddResource(renderPassResources.cameraBuffer);

        isSceneLoaded = true;
    }

    void VulkanRenderer::PrecomputeIBL()
    {
        irradianceMap = IrradianceMapGenerator(device).FromCubemapTexture(scene.skybox->GetCubemap());

        VkDescriptorImageInfo irradianceMapInfo{};
        irradianceMapInfo.sampler = irradianceMap->GetSampler();
        irradianceMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        irradianceMapInfo.imageView = irradianceMap->GetImageView();

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

                              UpdateLightsBuffer(deltaTime);
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
        VkDescriptorImageInfo renderImageInfo{};
        renderImageInfo.sampler = framebuffers.geometryFramebuffer->GetAttachments()[0].sampler;
        renderImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        renderImageInfo.imageView = framebuffers.geometryFramebuffer->GetAttachments()[0].imageView;

        auto descriptorSetLayout = ShaderCache::descriptorSetLayouts.presentDescriptorSetLayout;
        auto writer = VulkanDescriptorWriter(*descriptorSetLayout, device->GetShaderDescriptorPool());
        writer.WriteImage(0, &renderImageInfo);
        writer.BuildOrOverwrite(shaderCache->descriptorSets.presentDescriptorSet);
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
                    .SetRenderpass(shadowMapPass->GetRenderPass())
                    .SetResolution(SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION)
                    .AddAttachment(directionalShadowMap->GetImageView(i))
                    .Build();
        }
    }

    void VulkanRenderer::CreateFramebuffers()
    {
        framebuffers.ssaoFramebuffer = VulkanFramebuffer::Builder(device)
                        .SetRenderpass(ssaoPass->GetRenderPass())
                        .SetResolution(viewportResolution.width * 0.5, viewportResolution.height * 0.5)
                        .AddAttachment(ImageFormat::R8_UNORM)
                        .Build();

        framebuffers.geometryFramebuffer = VulkanFramebuffer::Builder(device)
                .SetRenderpass(lightingPass->GetRenderPass())
                .SetResolution(renderResolution.width, renderResolution.height)
                .AddAttachment(ImageFormat::RGBA16_SFLOAT)
                .AddAttachment(ImageFormat::DEPTH24_STENCIL8).Build();

        framebuffers.gbufferFramebuffer = VulkanFramebuffer::Builder(device)
                        .SetRenderpass(gbufferPass->GetRenderPass())
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
                    .SetRenderpass(presentPass->GetRenderPass())
                    .SetResolution(viewportResolution.width, viewportResolution.height)
                    .AddAttachment(vulkanSwapChain->GetImageViews()[i])
                    .Build();
        }
    }

    void VulkanRenderer::ResizeSwapchain()
    {
        IdleWait();
        CreateSwapchain();

        gbufferPass->Resize(renderResolution);
        skyboxPass->Resize(renderResolution);
        lightingPass->Resize(renderResolution);
        shadowMapPass->Resize(renderResolution);
        ssaoPass->Resize(renderResolution);
        gridPass->Resize(renderResolution);
        presentPass->Resize(viewportResolution);

        BuildGBuffer();
        CreateFramebuffers();
        CreatePresentDescriptorSet();

        PrepareSSAO();
        CreateRenderPassResources();
    }

    void VulkanRenderer::CreateCameraBuffer()
    {
        descriptorBuffers.cameraBuffer = CreateRef<VulkanBuffer>(device, sizeof(TransformsBuffer),
                                                                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                                                                 VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        VkDescriptorBufferInfo info{};
        info.buffer = descriptorBuffers.cameraBuffer->GetBuffer();
        info.offset = 0;
        info.range = sizeof(TransformsBuffer);

        VulkanDescriptorWriter(*shaderCache->descriptorSetLayouts.cameraDescriptorSetLayout, device->GetShaderDescriptorPool())
                .WriteBuffer(0, &info)
                .Build(shaderCache->descriptorSets.cameraDescriptorSet);


    }

    void VulkanRenderer::CreateLightsBuffer()
    {
        descriptorBuffers.lightsBuffer = CreateRef<VulkanBuffer>(device, sizeof(LightsBuffer),
                                                                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                                                                 VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                                                 VMA_MEMORY_USAGE_CPU_TO_GPU);

        VkDescriptorBufferInfo info{};
        info.buffer = descriptorBuffers.lightsBuffer->GetBuffer();
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

        memcpy(descriptorBuffers.cameraBuffer->GetData(), &bufferData, sizeof(TransformsBuffer));
    }

    void VulkanRenderer::RotateLight(float deltaTime)
    {
        Transform lightTransform;
        lightTransform.m_Rotation = glm::vec3(45.0f, 0.0f, 0.0f);

        const glm::mat4 rot_mat = rotate(glm::mat4(1.0f), glm::radians(22.5f * deltaTime), glm::vec3(0.0, 1.0, 0.0));
        scene.directionalLight.direction = normalize(glm::vec3(glm::vec4(scene.directionalLight.direction, 1.0f) * rot_mat));
    }

    void VulkanRenderer::UpdateLightsBuffer(float deltaTime)
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

        memcpy(descriptorBuffers.lightsBuffer->GetData(), &bufferData, sizeof(LightsBuffer));
    }

    void VulkanRenderer::DrawFrame(const VkCommandBuffer commandBuffer, const uint32_t imageIndex, Camera& camera)
    {
        activeImage = imageIndex;

        gbufferPass->Render(commandBuffer, camera, framebuffers.gbufferFramebuffer);
        directionalShadowMap->TransitionToDepthRendering(commandBuffer);
        for (size_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
        {
            shadowMapPass->SetCascadeIndex(i);
            shadowMapPass->Render(commandBuffer, camera, framebuffers.shadowMapFramebuffers[i]);
        }
        directionalShadowMap->TransitionToShaderRead(commandBuffer);

        ssaoPass->Render(commandBuffer, camera, framebuffers.ssaoFramebuffer);
        skyboxPass->Render(commandBuffer, camera, framebuffers.geometryFramebuffer);

        lightingPass->Render(commandBuffer, camera, framebuffers.geometryFramebuffer);
        gridPass->Render(commandBuffer, camera, framebuffers.geometryFramebuffer);

        presentPass->Render(commandBuffer, camera, framebuffers.presentFramebuffers[activeImage]);
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
        VkDescriptorImageInfo ssaoInfo{};
        ssaoInfo.sampler = framebuffers.ssaoFramebuffer->GetAttachments()[0].sampler;
        ssaoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        ssaoInfo.imageView = framebuffers.ssaoFramebuffer->GetAttachments()[0].imageView;

        VulkanDescriptorWriter(*ShaderCache::descriptorSetLayouts.postProcessingDescriptorSetLayout, device->GetShaderDescriptorPool())
                .WriteImage(0, &ssaoInfo)
                .BuildOrOverwrite(ShaderCache::descriptorSets.postProcessingDescriptorSet);
    }

    void VulkanRenderer::CreateRenderPassResources()
    {
        renderPassResources.viewspaceNormal = {
            .name = "viewspace_normal",
            .type = ResourceType::Texture,
            .descriptorSet = shaderCache->descriptorSets.viewspaceNormalDescriptorSet,
            .descriptorSetLayout = shaderCache->descriptorSetLayouts.viewspaceNormalDescriptorSetLayout
        };

        renderPassResources.viewspacePosition = {
            .name = "viewspace_position",
            .type = ResourceType::Texture,
            .descriptorSet = shaderCache->descriptorSets.viewspacePositionDescriptorSet,
            .descriptorSetLayout = shaderCache->descriptorSetLayouts.viewspacePositionDescriptorSetLayout
        };

        renderPassResources.depthMap = {
            .name = "depth_map",
            .type = ResourceType::Texture,
            .descriptorSet = shaderCache->descriptorSets.depthMapDescriptorSet,
            .descriptorSetLayout = shaderCache->descriptorSetLayouts.depthMapDescriptorSetLayout
        };

        renderPassResources.lightsBuffer = {
            .name = "lights_buffer",
            .type = ResourceType::Buffer,
            .descriptorSet = shaderCache->descriptorSets.lightsDescriptorSet,
            .descriptorSetLayout = shaderCache->descriptorSetLayouts.lightsDescriptorSetLayout
        };

        renderPassResources.directionalShadowMap = {
            .name = "directional_shadow_map",
            .type = ResourceType::Texture,
            .descriptorSet = shaderCache->descriptorSets.directionalShadownMapDescriptorSet,
            .descriptorSetLayout = shaderCache->descriptorSetLayouts.directionalShadowMapDescriptorSetLayout
        };

        renderPassResources.cameraBuffer = {
            .name = "camera_buffer",
            .type = ResourceType::Buffer,
            .descriptorSet = shaderCache->descriptorSets.cameraDescriptorSet,
            .descriptorSetLayout = shaderCache->descriptorSetLayouts.cameraDescriptorSetLayout
        };

        renderPassResources.irradianceMap = {
            .name = "irradiance_map",
            .type = ResourceType::Texture,
            .descriptorSet = shaderCache->descriptorSets.irradianceDescriptorSet,
            .descriptorSetLayout = shaderCache->descriptorSetLayouts.irradianceDescriptorSetLayout
        };

        renderPassResources.ssaoTexture = {
            .name = "ssao_texture",
            .type = ResourceType::Texture,
            .descriptorSet = shaderCache->descriptorSets.postProcessingDescriptorSet,
            .descriptorSetLayout = shaderCache->descriptorSetLayouts.postProcessingDescriptorSetLayout
        };
    }
}
