#include "renderer/vulkan/vulkan_renderer.h"

#include <renderer/vulkan/pass/lighting_pass.h>
#include <renderer/vulkan/pass/lighting/brdf_lut_pass.h>

#include "renderer/vulkan/vulkan_descriptor_writer.h"
#include "renderer/vulkan/vulkan_framebuffer.h"
#include "renderer/vulkan/vulkan_device.h"
#include "renderer/vulkan/vulkan_mesh.h"
#include "renderer/vulkan/vulkan_shader_compiler.h"

#include "imgui.h"

#include "resource/resource_manager.h"

#include "util/log.h"
#include "util/filesystem.h"

namespace MongooseVK
{
    VulkanRenderer::~VulkanRenderer()
    {
        device->DestroyBuffer(renderPassResources.cameraBuffer.bufferInfo.allocatedBuffer);
        device->DestroyBuffer(renderPassResources.cameraBuffer.bufferInfo.allocatedBuffer);
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

        CreateRenderPassTextures();
        CreateRenderPassResources();
        CreateRenderPassBuffers();

        renderPasses.gbufferPass = CreateScope<GBufferPass>(device, scene, renderResolution);
        renderPasses.skyboxPass = CreateScope<SkyboxPass>(device, scene, renderResolution);
        renderPasses.lightingPass = CreateScope<LightingPass>(device, scene, renderResolution);
        renderPasses.shadowMapPass = CreateScope<ShadowMapPass>(device, scene, renderResolution);
        renderPasses.presentPass = CreateScope<PresentPass>(device, renderResolution);
        renderPasses.ssaoPass = CreateScope<SSAOPass>(device, renderResolution);
        renderPasses.gridPass = CreateScope<InfiniteGridPass>(device, renderResolution);
        renderPasses.irradianceMapPass = CreateScope<IrradianceMapPass>(device, renderResolution);
        renderPasses.brdfLutPass = CreateScope<BrdfLUTPass>(device, renderResolution);
        renderPasses.prefilterMapPass = CreateScope<PrefilterMapPass>(device, renderResolution);

        CreateShadowMap();
        CreateFramebuffers();

        PrecomputeIBL();
        CalculateBrdfLUT();
        CalculatePrefilterMap();

        isSceneLoaded = true;
    }

    void VulkanRenderer::CalculateBrdfLUT()
    {
        const VulkanTexture* brdfLutTexture = device->GetTexture(renderPassResources.brdfLutTexture.textureInfo.textureHandle);

        FramebufferCreateInfo createInfo{};

        createInfo.attachments.push_back({brdfLutTexture->GetImageView()});
        createInfo.renderPassHandle = renderPasses.brdfLutPass->GetRenderPassHandle();
        createInfo.resolution = {512, 512};

        const auto brdfLutFramebufferHandle = device->CreateFramebuffer(createInfo);

        device->ImmediateSubmit([&](VkCommandBuffer commandBuffer) {
            renderPasses.brdfLutPass->Render(commandBuffer, nullptr, brdfLutFramebufferHandle);
        });

        device->DestroyFramebuffer(brdfLutFramebufferHandle);
    }

    void VulkanRenderer::CalculatePrefilterMap()
    {
        device->ImmediateSubmit([&](const VkCommandBuffer commandBuffer) {
            renderPasses.prefilterMapPass->SetCubemapTexture(renderPassResources.skyboxTexture.textureInfo.textureHandle);
            renderPasses.prefilterMapPass->SetTargetTexture(renderPassResources.prefilterMapTexture.textureInfo.textureHandle);
            renderPasses.prefilterMapPass->Render(commandBuffer, nullptr, INVALID_FRAMEBUFFER_HANDLE);
        });
    }

    void VulkanRenderer::PrecomputeIBL()
    {
        VulkanTexture* irradianceMapTexture = device->GetTexture(renderPassResources.irradianceMapTexture.textureInfo.textureHandle);

        std::vector<FramebufferHandle> iblIrradianceFramebuffes;
        iblIrradianceFramebuffes.resize(6);

        for (size_t i = 0; i < 6; i++)
        {
            FramebufferCreateInfo createInfo{};

            createInfo.attachments.push_back({irradianceMapTexture->GetMipmapImageView(0, i)});
            createInfo.renderPassHandle = renderPasses.irradianceMapPass->GetRenderPassHandle();
            createInfo.resolution = {32, 32};

            iblIrradianceFramebuffes[i] = device->CreateFramebuffer(createInfo);
        }

        device->ImmediateSubmit([&](const VkCommandBuffer commandBuffer) {
            for (size_t faceIndex = 0; faceIndex < 6; faceIndex++)
            {
                renderPasses.irradianceMapPass->SetFaceIndex(faceIndex);
                renderPasses.irradianceMapPass->Render(commandBuffer, nullptr, iblIrradianceFramebuffes[faceIndex]);
            }
        });

        const VkDescriptorImageInfo irradianceMapInfo = {
            .sampler = irradianceMapTexture->GetSampler(),
            .imageView = irradianceMapTexture->GetImageView(),
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };

        VulkanDescriptorWriter(*shaderCache->descriptorSetLayouts.irradianceDescriptorSetLayout, device->GetShaderDescriptorPool())
                .WriteImage(0, &irradianceMapInfo)
                .Build(shaderCache->descriptorSets.irradianceDescriptorSet);
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

    void VulkanRenderer::CreateShadowMap()
    {
        VulkanTexture* shadowMapTexture = device->GetTexture(renderPassResources.directionalShadowMap.textureInfo.textureHandle);
        const uint16_t SHADOW_MAP_RESOLUTION = EnumValue(scene.directionalLight.shadowMapResolution);

        framebuffers.shadowMapFramebuffers.resize(SHADOW_MAP_CASCADE_COUNT);
        for (size_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
        {
            FramebufferCreateInfo createInfo{};
            createInfo.attachments.push_back({shadowMapTexture->GetImageView(i)});
            createInfo.renderPassHandle = renderPasses.shadowMapPass->GetRenderPassHandle();
            createInfo.resolution = {SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION};

            framebuffers.shadowMapFramebuffers[i] = device->CreateFramebuffer(createInfo);
        }
    }

    void VulkanRenderer::CreateFramebuffers()
    {
        // SSAO
        {
            FramebufferCreateInfo createInfo{};
            createInfo.attachments.push_back({.textureHandle = renderPassResources.ssaoTexture.textureInfo.textureHandle});
            createInfo.renderPassHandle = renderPasses.ssaoPass->GetRenderPassHandle();
            createInfo.resolution = {
                static_cast<uint32_t>(renderResolution.width * 0.5),
                static_cast<uint32_t>(renderResolution.height * 0.5)
            };

            framebuffers.ssaoFramebuffer = device->CreateFramebuffer(createInfo);
        }

        // Main frame
        {
            FramebufferCreateInfo createInfo{};
            createInfo.attachments.push_back({.textureHandle = renderPassResources.mainFrameColorTexture.textureInfo.textureHandle});
            createInfo.attachments.push_back({.textureHandle = renderPassResources.depthMap.textureInfo.textureHandle});
            createInfo.renderPassHandle = renderPasses.lightingPass->GetRenderPassHandle();
            createInfo.resolution = {renderResolution.width, renderResolution.height};

            framebuffers.mainFramebuffer = device->CreateFramebuffer(createInfo);
        }

        // GBuffer
        {
            FramebufferCreateInfo createInfo{};
            createInfo.attachments.push_back({.textureHandle = renderPassResources.viewspaceNormal.textureInfo.textureHandle});
            createInfo.attachments.push_back({.textureHandle = renderPassResources.viewspacePosition.textureInfo.textureHandle});
            createInfo.attachments.push_back({.textureHandle = renderPassResources.depthMap.textureInfo.textureHandle});
            createInfo.renderPassHandle = renderPasses.gbufferPass->GetRenderPassHandle();
            createInfo.resolution = {renderResolution.width, renderResolution.height};

            framebuffers.gbufferFramebuffer = device->CreateFramebuffer(createInfo);
        }

        // Present
        {
            const uint32_t imageCount = VulkanUtils::GetSwapchainImageCount(device->GetPhysicalDevice(), device->GetSurface());
            framebuffers.presentFramebuffers.clear();
            framebuffers.presentFramebuffers.resize(imageCount);
            for (size_t i = 0; i < imageCount; i++)
            {
                FramebufferCreateInfo createInfo{};
                createInfo.attachments.push_back({.imageView = vulkanSwapChain->GetImageViews()[i]});
                createInfo.renderPassHandle = renderPasses.presentPass->GetRenderPassHandle();
                createInfo.resolution = {viewportResolution.width, viewportResolution.height};

                framebuffers.presentFramebuffers[i] = device->CreateFramebuffer(createInfo);
            }
        }
    }

    void VulkanRenderer::ResizeSwapchain()
    {
        IdleWait();

        CreateSwapchain();
        CreateRenderPassResources();

        renderPasses.gbufferPass->Resize(renderResolution);
        renderPasses.skyboxPass->Resize(renderResolution);
        renderPasses.lightingPass->Resize(renderResolution);
        renderPasses.shadowMapPass->Resize(renderResolution);
        renderPasses.ssaoPass->Resize(renderResolution);
        renderPasses.gridPass->Resize(renderResolution);
        renderPasses.presentPass->Resize(viewportResolution);
        renderPasses.brdfLutPass->Resize(viewportResolution);
        renderPasses.prefilterMapPass->Resize(viewportResolution);

        CreateFramebuffers();
    }

    void VulkanRenderer::UpdateCameraBuffer(Camera& camera) const
    {
        CameraBuffer bufferData;
        bufferData.cameraPosition = camera.GetTransform().m_Position;
        bufferData.view = camera.GetView();
        bufferData.proj = camera.GetProjection();

        memcpy(renderPassResources.cameraBuffer.bufferInfo.allocatedBuffer.GetData(), &bufferData, sizeof(CameraBuffer));
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

        memcpy(renderPassResources.lightsBuffer.bufferInfo.allocatedBuffer.GetData(), &bufferData, sizeof(LightsBuffer));
    }

    void VulkanRenderer::DrawFrame(const VkCommandBuffer commandBuffer, const uint32_t imageIndex, Camera& camera)
    {
        activeImage = imageIndex;

        renderPasses.gbufferPass->Render(commandBuffer, &camera, framebuffers.gbufferFramebuffer);
        // TODO Don't forget to add image barriers back later
        //directionalShadowMap->TransitionToDepthRendering(commandBuffer);
        for (size_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
        {
            renderPasses.shadowMapPass->SetCascadeIndex(i);
            renderPasses.shadowMapPass->Render(commandBuffer, &camera, framebuffers.shadowMapFramebuffers[i]);
        }
        //directionalShadowMap->TransitionToShaderRead(commandBuffer);

        renderPasses.ssaoPass->Render(commandBuffer, &camera, framebuffers.ssaoFramebuffer);

        VulkanTexture* depthMap = device->GetTexture(renderPassResources.depthMap.textureInfo.textureHandle);

        renderPasses.skyboxPass->Render(commandBuffer, &camera, framebuffers.mainFramebuffer);
        renderPasses.gridPass->Render(commandBuffer, &camera, framebuffers.mainFramebuffer);

        VulkanUtils::TransitionImageLayout(commandBuffer, depthMap->GetImage(),
                                           VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
                                           VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                           VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);

        renderPasses.lightingPass->Render(commandBuffer, &camera, framebuffers.mainFramebuffer);

        VulkanUtils::TransitionImageLayout(commandBuffer, depthMap->GetImage(),
                                           VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
                                           VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
                                           VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

        renderPasses.presentPass->Render(commandBuffer, &camera, framebuffers.presentFramebuffers[activeImage]);
    }

    void VulkanRenderer::CreateRenderPassResources()
    {
        // Viewspace Normal
        {
            ResourceTextureInfo textureInfo;
            textureInfo.textureCreateInfo = {
                .width = renderResolution.width,
                .height = renderResolution.height,
                .format = ImageFormat::RGBA32_SFLOAT,
                .imageLayout = ImageUtils::GetLayoutFromFormat(ImageFormat::RGBA32_SFLOAT),
                .addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            };

            textureInfo.textureHandle = device->CreateTexture(textureInfo.textureCreateInfo);

            renderPassResources.viewspaceNormal = {
                .name = "viewspace_normal",
                .type = ResourceType::Texture,
                .textureInfo = textureInfo,
            };

            VulkanTexture* viewspaceNormalTexture = device->GetTexture(renderPassResources.viewspaceNormal.textureInfo.textureHandle);
            const VkDescriptorImageInfo worldSpaceNormalInfo{
                .sampler = viewspaceNormalTexture->sampler,
                .imageView = viewspaceNormalTexture->imageView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };

            VulkanDescriptorWriter(*ShaderCache::descriptorSetLayouts.viewspaceNormalDescriptorSetLayout, device->GetShaderDescriptorPool())
                    .WriteImage(0, &worldSpaceNormalInfo)
                    .BuildOrOverwrite(ShaderCache::descriptorSets.viewspaceNormalDescriptorSet);
        }

        // Viewspace Position
        {
            ResourceTextureInfo textureInfo;
            textureInfo.textureCreateInfo = {
                .width = renderResolution.width,
                .height = renderResolution.height,
                .format = ImageFormat::RGBA32_SFLOAT,
                .imageLayout = ImageUtils::GetLayoutFromFormat(ImageFormat::RGBA32_SFLOAT),
                .addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            };

            textureInfo.textureHandle = device->CreateTexture(textureInfo.textureCreateInfo);

            renderPassResources.viewspacePosition = {
                .name = "viewspace_position",
                .type = ResourceType::Texture,
                .textureInfo = textureInfo,
            };

            VulkanTexture* viewspacePositionTexture = device->GetTexture(renderPassResources.viewspacePosition.textureInfo.textureHandle);
            const VkDescriptorImageInfo positionInfo{
                .sampler = viewspacePositionTexture->sampler,
                .imageView = viewspacePositionTexture->imageView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };

            VulkanDescriptorWriter(*ShaderCache::descriptorSetLayouts.viewspacePositionDescriptorSetLayout,
                                   device->GetShaderDescriptorPool())
                    .WriteImage(0, &positionInfo)
                    .BuildOrOverwrite(ShaderCache::descriptorSets.viewspacePositionDescriptorSet);
        }

        // Depth Map
        {
            ResourceTextureInfo textureInfo;
            textureInfo.textureCreateInfo = {
                .width = renderResolution.width,
                .height = renderResolution.height,
                .format = ImageFormat::DEPTH24_STENCIL8,
                .imageLayout = ImageUtils::GetLayoutFromFormat(ImageFormat::DEPTH24_STENCIL8),
                .addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            };

            textureInfo.textureHandle = device->CreateTexture(textureInfo.textureCreateInfo);

            renderPassResources.depthMap = {
                .name = "depth_map",
                .type = ResourceType::Texture,
                .textureInfo = textureInfo,
            };

            VulkanTexture* depthTexture = device->GetTexture(renderPassResources.depthMap.textureInfo.textureHandle);
            const VkDescriptorImageInfo depthInfo{
                .sampler = depthTexture->sampler,
                .imageView = depthTexture->imageView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };

            VulkanDescriptorWriter(*ShaderCache::descriptorSetLayouts.depthMapDescriptorSetLayout, device->GetShaderDescriptorPool())
                    .WriteImage(0, &depthInfo)
                    .BuildOrOverwrite(ShaderCache::descriptorSets.depthMapDescriptorSet);
        }

        // Directional Shadow Map
        {
            const uint16_t SHADOW_MAP_RESOLUTION = EnumValue(scene.directionalLight.shadowMapResolution);

            ResourceTextureInfo textureInfo;
            textureInfo.textureCreateInfo = {
                .width = SHADOW_MAP_RESOLUTION,
                .height = SHADOW_MAP_RESOLUTION,
                .format = ImageFormat::DEPTH32,
                .imageLayout = ImageUtils::GetLayoutFromFormat(ImageFormat::DEPTH32),
                .addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
                .arrayLayers = SHADOW_MAP_CASCADE_COUNT,
                .compareEnabled = true,
                .compareOp = VK_COMPARE_OP_LESS
            };

            textureInfo.textureHandle = device->CreateTexture(textureInfo.textureCreateInfo);

            renderPassResources.directionalShadowMap = {
                .name = "directional_shadow_map",
                .type = ResourceType::Texture,
                .textureInfo = textureInfo,
            };
        }

        // SSAO Texture
        {
            ResourceTextureInfo textureInfo;
            textureInfo.textureCreateInfo = {
                .width = static_cast<uint32_t>(renderResolution.width * 0.5),
                .height = static_cast<uint32_t>(renderResolution.height * 0.5),
                .format = ImageFormat::R8_UNORM,
                .imageLayout = ImageUtils::GetLayoutFromFormat(ImageFormat::R8_UNORM),
            };

            textureInfo.textureHandle = device->CreateTexture(textureInfo.textureCreateInfo);

            renderPassResources.ssaoTexture = {
                .name = "ssao_texture",
                .type = ResourceType::Texture,
                .textureInfo = textureInfo,
            };

            VulkanTexture* ssaoTexture = device->GetTexture(renderPassResources.ssaoTexture.textureInfo.textureHandle);
            const VkDescriptorImageInfo ssaoInfo = {
                .sampler = ssaoTexture->sampler,
                .imageView = ssaoTexture->imageView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };

            VulkanDescriptorWriter(*ShaderCache::descriptorSetLayouts.postProcessingDescriptorSetLayout, device->GetShaderDescriptorPool())
                    .WriteImage(0, &ssaoInfo)
                    .BuildOrOverwrite(ShaderCache::descriptorSets.postProcessingDescriptorSet);
        }

        // Main Frame Color
        {
            ResourceTextureInfo textureInfo;
            textureInfo.textureCreateInfo = {
                .width = renderResolution.width,
                .height = renderResolution.height,
                .format = ImageFormat::RGBA16_SFLOAT,
                .imageLayout = ImageUtils::GetLayoutFromFormat(ImageFormat::RGBA16_SFLOAT),
                .addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            };

            textureInfo.textureHandle = device->CreateTexture(textureInfo.textureCreateInfo);

            renderPassResources.mainFrameColorTexture = {
                .name = "main_frame_color",
                .type = ResourceType::Texture,
                .textureInfo = textureInfo,
            };

            VulkanTexture* mainFrameColorTexture = device->GetTexture(renderPassResources.mainFrameColorTexture.textureInfo.textureHandle);
            const VkDescriptorImageInfo renderImageInfo = {
                .sampler = mainFrameColorTexture->sampler,
                .imageView = mainFrameColorTexture->imageView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };

            VulkanDescriptorWriter(*ShaderCache::descriptorSetLayouts.presentDescriptorSetLayout, device->GetShaderDescriptorPool())
                    .WriteImage(0, &renderImageInfo)
                    .BuildOrOverwrite(shaderCache->descriptorSets.presentDescriptorSet);
        }
    }

    void VulkanRenderer::CreateRenderPassBuffers()
    {
        // Lights Buffer
        {
            ResourceBufferInfo bufferInfo;
            bufferInfo.size = sizeof(LightsBuffer);

            bufferInfo.allocatedBuffer = {};
            renderPassResources.lightsBuffer = {
                .name = "lights_buffer",
                .type = ResourceType::Buffer,
                .bufferInfo = bufferInfo,
            };

            renderPassResources.lightsBuffer.bufferInfo.allocatedBuffer = device->CreateBuffer(sizeof(LightsBuffer),
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                VMA_MEMORY_USAGE_CPU_TO_GPU);

            VkDescriptorBufferInfo info{};
            info.buffer = renderPassResources.lightsBuffer.bufferInfo.allocatedBuffer.buffer;
            info.offset = 0;
            info.range = sizeof(LightsBuffer);

            VulkanTexture* shadowMapTexture = device->GetTexture(renderPassResources.directionalShadowMap.textureInfo.textureHandle);
            VkDescriptorImageInfo shadowMapInfo = {
                .sampler = shadowMapTexture->GetSampler(),
                .imageView = shadowMapTexture->GetImageView(),
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };

            VulkanDescriptorWriter(*shaderCache->descriptorSetLayouts.lightsDescriptorSetLayout, device->GetShaderDescriptorPool())
                    .WriteBuffer(0, &info)
                    .Build(shaderCache->descriptorSets.lightsDescriptorSet);

            VulkanDescriptorWriter(*shaderCache->descriptorSetLayouts.directionalShadowMapDescriptorSetLayout,
                                   device->GetShaderDescriptorPool())
                    .WriteImage(0, &shadowMapInfo)
                    .Build(shaderCache->descriptorSets.directionalShadownMapDescriptorSet);
        }

        // Camera Buffer
        {
            ResourceBufferInfo bufferInfo;
            bufferInfo.size = sizeof(CameraBuffer);

            bufferInfo.allocatedBuffer = {};
            renderPassResources.cameraBuffer = {
                .name = "camera_buffer",
                .type = ResourceType::Buffer,
                .bufferInfo = bufferInfo,
            };

            renderPassResources.cameraBuffer.bufferInfo.allocatedBuffer = device->CreateBuffer(sizeof(CameraBuffer),
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                VMA_MEMORY_USAGE_CPU_TO_GPU);

            VkDescriptorBufferInfo info{};
            info.buffer = renderPassResources.cameraBuffer.bufferInfo.allocatedBuffer.buffer;
            info.offset = 0;
            info.range = sizeof(CameraBuffer);

            VulkanDescriptorWriter(*shaderCache->descriptorSetLayouts.cameraDescriptorSetLayout, device->GetShaderDescriptorPool())
                    .WriteBuffer(0, &info)
                    .Build(shaderCache->descriptorSets.cameraDescriptorSet);
        }
    }

    void VulkanRenderer::CreateRenderPassTextures()
    {
        // Skybox
        {
            Bitmap cubemapBitmap = ResourceManager::LoadHDRCubeMapBitmap(device, scene.skyboxPath);
            const TextureCreateInfo textureCreateInfo = {
                .width = cubemapBitmap.width,
                .height = cubemapBitmap.height,
                .format = ImageFormat::RGBA32_SFLOAT,
                .imageLayout = ImageUtils::GetLayoutFromFormat(ImageFormat::RGBA32_SFLOAT),
                .addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                .arrayLayers = 6,
                .flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
                .isCubeMap = true,
            };

            ResourceTextureInfo textureInfo{};
            textureInfo.textureHandle = device->CreateTexture(textureCreateInfo);
            device->UploadCubemapTextureData(textureInfo.textureHandle, &cubemapBitmap);

            renderPassResources.skyboxTexture = {
                .name = "skybox_texture",
                .type = ResourceType::TextureCube,
                .textureInfo = textureInfo,
            };

            VulkanTexture* skyboxTexture = device->GetTexture(renderPassResources.skyboxTexture.textureInfo.textureHandle);

            VkDescriptorImageInfo info{
                skyboxTexture->GetSampler(),
                skyboxTexture->GetImageView(),
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            };

            VulkanDescriptorWriter(*ShaderCache::descriptorSetLayouts.cubemapDescriptorSetLayout, device->GetShaderDescriptorPool())
                    .WriteImage(0, &info)
                    .Build(ShaderCache::descriptorSets.cubemapDescriptorSet);
        }

        // BRDF LUT
        {
            ResourceTextureInfo textureInfo{};
            TextureCreateInfo textureCreateInfo = {};
            textureCreateInfo.width = 512;
            textureCreateInfo.height = 512;
            textureCreateInfo.format = ImageFormat::RGBA16_SFLOAT;
            textureCreateInfo.imageLayout = ImageUtils::GetLayoutFromFormat(ImageFormat::RGBA16_SFLOAT);

            textureInfo.textureHandle = device->CreateTexture(textureCreateInfo);

            renderPassResources.brdfLutTexture = {
                .name = "brdflut_texture",
                .type = ResourceType::Texture,
                .textureInfo = textureInfo,
            };

            VulkanTexture* brdfLUTTexture = device->GetTexture(renderPassResources.brdfLutTexture.textureInfo.textureHandle);
            VkDescriptorImageInfo brdfLUTInfo{};
            brdfLUTInfo.sampler = brdfLUTTexture->GetSampler();
            brdfLUTInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            brdfLUTInfo.imageView = brdfLUTTexture->GetImageView();

            VulkanDescriptorWriter(*ShaderCache::descriptorSetLayouts.brdfLutDescriptorSetLayout, device->GetShaderDescriptorPool())
                    .WriteImage(0, &brdfLUTInfo)
                    .Build(ShaderCache::descriptorSets.brdfLutDescriptorSet);
        }

        // Prefilter Map
        {
            constexpr uint32_t PREFILTER_MIP_LEVELS = 6;
            constexpr uint32_t REFLECTION_RESOLUTION = 256;

            ResourceTextureInfo textureInfo{};
            TextureCreateInfo textureCreateInfo = {};
            textureCreateInfo.width = REFLECTION_RESOLUTION;
            textureCreateInfo.height = REFLECTION_RESOLUTION;
            textureCreateInfo.format = ImageFormat::RGBA16_SFLOAT;
            textureCreateInfo.imageLayout = ImageUtils::GetLayoutFromFormat(ImageFormat::RGBA16_SFLOAT);
            textureCreateInfo.addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            textureCreateInfo.mipLevels = PREFILTER_MIP_LEVELS;
            textureCreateInfo.arrayLayers = 6;
            textureCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
            textureCreateInfo.isCubeMap = true;

            textureInfo.textureHandle = device->CreateTexture(textureCreateInfo);

            renderPassResources.prefilterMapTexture = {
                .name = "prefilter_map_texture",
                .type = ResourceType::TextureCube,
                .textureInfo = textureInfo,
            };

            VulkanTexture* prefilterMapTexture = device->GetTexture(renderPassResources.prefilterMapTexture.textureInfo.textureHandle);
            VkDescriptorImageInfo prefilterMapInfo{};
            prefilterMapInfo.sampler = prefilterMapTexture->GetSampler();
            prefilterMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            prefilterMapInfo.imageView = prefilterMapTexture->GetImageView();

            VulkanDescriptorWriter(*ShaderCache::descriptorSetLayouts.reflectionDescriptorSetLayout, device->GetShaderDescriptorPool())
                    .WriteImage(0, &prefilterMapInfo)
                    .Build(ShaderCache::descriptorSets.reflectionDescriptorSet);
        }

        // Irradiance Map
        {
            ResourceTextureInfo textureInfo;
            textureInfo.textureCreateInfo = {};
            textureInfo.textureCreateInfo.width = 32;
            textureInfo.textureCreateInfo.height = 32;
            textureInfo.textureCreateInfo.format = ImageFormat::RGBA16_SFLOAT;
            textureInfo.textureCreateInfo.imageLayout = ImageUtils::GetLayoutFromFormat(ImageFormat::RGBA16_SFLOAT);
            textureInfo.textureCreateInfo.addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            textureInfo.textureCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
            textureInfo.textureCreateInfo.isCubeMap = true;
            textureInfo.textureCreateInfo.arrayLayers = 6;

            textureInfo.textureHandle = device->CreateTexture(textureInfo.textureCreateInfo);

            renderPassResources.irradianceMapTexture = {
                .name = "irradiance_map_texture",
                .type = ResourceType::TextureCube,
                .textureInfo = textureInfo,
            };
        }
    }
}
