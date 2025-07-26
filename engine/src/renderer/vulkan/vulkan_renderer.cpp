#include "renderer/vulkan/vulkan_renderer.h"

#include <renderer/vulkan/pass/lighting_pass.h>
#include <renderer/vulkan/pass/lighting/brdf_lut_pass.h>

#include "renderer/vulkan/vulkan_descriptor_writer.h"
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
        device->DestroyBuffer(renderPassResources.cameraBuffer.resourceInfo.buffer.allocatedBuffer);
        device->DestroyBuffer(renderPassResources.cameraBuffer.resourceInfo.buffer.allocatedBuffer);
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

        renderPasses.lightingPass = CreateScope<LightingPass>(device, scene, renderResolution);
        renderPasses.shadowMapPass = CreateScope<ShadowMapPass>(device, scene, renderResolution);
        renderPasses.presentPass = CreateScope<PresentPass>(device, renderResolution);
        renderPasses.ssaoPass = CreateScope<SSAOPass>(device, VkExtent2D{
                                                          static_cast<uint32_t>(renderResolution.width * 0.5),
                                                          static_cast<uint32_t>(renderResolution.height * 0.5)
                                                      });
        renderPasses.irradianceMapPass = CreateScope<IrradianceMapPass>(device, renderResolution);
        renderPasses.brdfLutPass = CreateScope<BrdfLUTPass>(device, VkExtent2D{512, 512});
        renderPasses.prefilterMapPass = CreateScope<PrefilterMapPass>(device, renderResolution);
        renderPasses.skyboxPass = CreateScope<SkyboxPass>(device, scene, renderResolution);
        renderPasses.gridPass = CreateScope<InfiniteGridPass>(device, renderResolution);

        // Skybox pass
        {
            renderPasses.skyboxPass->Reset();

            renderPasses.skyboxPass->AddInput({
                renderPassResources.skyboxTexture
            });
            renderPasses.skyboxPass->AddInput({
                renderPassResources.cameraBuffer
            });

            renderPasses.skyboxPass->AddOutput({
                .resource = renderPassResources.mainFrameColorTexture,
                .loadOp = RenderPassOperation::LoadOp::Clear,
                .storeOp = RenderPassOperation::StoreOp::Store
            });
            renderPasses.skyboxPass->AddOutput({
                .resource = renderPassResources.depthMap,
                .loadOp = RenderPassOperation::LoadOp::Clear,
                .storeOp = RenderPassOperation::StoreOp::Store
            });

            renderPasses.skyboxPass->Init();
        }

        // Grid pass
        {
            renderPasses.gridPass->Reset();

            renderPasses.gridPass->AddInput({
                renderPassResources.cameraBuffer
            });

            renderPasses.gridPass->AddOutput({
                .resource = renderPassResources.mainFrameColorTexture,
                .loadOp = RenderPassOperation::LoadOp::Load,
                .storeOp = RenderPassOperation::StoreOp::Store
            });

            renderPasses.gridPass->AddOutput({
                .resource = renderPassResources.depthMap,
                .loadOp = RenderPassOperation::LoadOp::Load,
                .storeOp = RenderPassOperation::StoreOp::Store
            });

            renderPasses.gridPass->Init();
        }

        // BRDF LUT pass
        {
            renderPasses.brdfLutPass->Reset();

            renderPasses.brdfLutPass->AddOutput({
                .resource = renderPassResources.brdfLutTexture,
                .loadOp = RenderPassOperation::LoadOp::Clear,
                .storeOp = RenderPassOperation::StoreOp::Store
            });

            renderPasses.brdfLutPass->Init();
        }

        // GBuffer pass
        {
            renderPasses.gbufferPass->Reset();

            renderPasses.gbufferPass->AddInput({
                renderPassResources.cameraBuffer
            });

            renderPasses.gbufferPass->AddOutput({
                .resource = renderPassResources.viewspaceNormal,
                .loadOp = RenderPassOperation::LoadOp::Clear,
                .storeOp = RenderPassOperation::StoreOp::Store
            });

            renderPasses.gbufferPass->AddOutput({
                .resource = renderPassResources.viewspacePosition,
                .loadOp = RenderPassOperation::LoadOp::Clear,
                .storeOp = RenderPassOperation::StoreOp::Store
            });

            renderPasses.gbufferPass->AddOutput({
                .resource = renderPassResources.depthMap,
                .loadOp = RenderPassOperation::LoadOp::Clear,
                .storeOp = RenderPassOperation::StoreOp::Store
            });

            renderPasses.gbufferPass->Init();
        }

        // Lighting pass
        {
            renderPasses.lightingPass->Reset();

            renderPasses.lightingPass->AddInput({renderPassResources.cameraBuffer});
            renderPasses.lightingPass->AddInput({renderPassResources.lightsBuffer});
            renderPasses.lightingPass->AddInput({renderPassResources.directionalShadowMap});
            renderPasses.lightingPass->AddInput({renderPassResources.irradianceMapTexture});
            renderPasses.lightingPass->AddInput({renderPassResources.ssaoTexture});
            renderPasses.lightingPass->AddInput({renderPassResources.prefilterMapTexture});
            renderPasses.lightingPass->AddInput({renderPassResources.brdfLutTexture});

            renderPasses.lightingPass->AddOutput({
                .resource = renderPassResources.mainFrameColorTexture,
                .loadOp = RenderPassOperation::LoadOp::Load,
                .storeOp = RenderPassOperation::StoreOp::Store
            });

            renderPasses.lightingPass->AddOutput({
                .resource = renderPassResources.depthMap,
                .loadOp = RenderPassOperation::LoadOp::Load,
                .storeOp = RenderPassOperation::StoreOp::Store
            });

            renderPasses.lightingPass->Init();
        }

        CreateShadowMap();
        CreateFramebuffers();

        PrecomputeIBL();
        CalculateBrdfLUT();
        CalculatePrefilterMap();

        isSceneLoaded = true;
    }

    void VulkanRenderer::CalculateBrdfLUT()
    {
        device->ImmediateSubmit([&](VkCommandBuffer commandBuffer) {
            renderPasses.brdfLutPass->Render(commandBuffer, nullptr, INVALID_FRAMEBUFFER_HANDLE);
        });
    }

    void VulkanRenderer::CalculatePrefilterMap()
    {
        device->ImmediateSubmit([&](const VkCommandBuffer commandBuffer) {
            renderPasses.prefilterMapPass->SetCubemapTexture(renderPassResources.skyboxTexture.resourceInfo.texture.textureHandle);
            renderPasses.prefilterMapPass->SetTargetTexture(renderPassResources.prefilterMapTexture.resourceInfo.texture.textureHandle);
            renderPasses.prefilterMapPass->Render(commandBuffer, nullptr, INVALID_FRAMEBUFFER_HANDLE);
        });
    }

    void VulkanRenderer::PrecomputeIBL()
    {
        VulkanTexture* irradianceMapTexture = device->GetTexture(
            renderPassResources.irradianceMapTexture.resourceInfo.texture.textureHandle);

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

        auto descriptorSetLayoutHandle = device->GetDescriptorSetLayout(shaderCache->descriptorSetLayouts.irradianceDescriptorSetLayout);
        VulkanDescriptorWriter(*descriptorSetLayoutHandle, device->GetShaderDescriptorPool())
                .WriteImage(0, irradianceMapInfo)
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
        VulkanTexture* shadowMapTexture = device->GetTexture(renderPassResources.directionalShadowMap.resourceInfo.texture.textureHandle);
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
            createInfo.attachments.push_back({.textureHandle = renderPassResources.ssaoTexture.resourceInfo.texture.textureHandle});
            createInfo.renderPassHandle = renderPasses.ssaoPass->GetRenderPassHandle();
            createInfo.resolution = {
                static_cast<uint32_t>(renderResolution.width * 0.5),
                static_cast<uint32_t>(renderResolution.height * 0.5)
            };

            framebuffers.ssaoFramebuffer = device->CreateFramebuffer(createInfo);
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
        renderPasses.ssaoPass->Resize(renderResolution);
        renderPasses.gridPass->Resize(renderResolution);
        renderPasses.presentPass->Resize(viewportResolution);

        // Skybox pass
        {
            renderPasses.skyboxPass->Reset();

            renderPasses.skyboxPass->AddInput({
                renderPassResources.skyboxTexture
            });
            renderPasses.skyboxPass->AddInput({
                renderPassResources.cameraBuffer
            });

            renderPasses.skyboxPass->AddOutput({
                .resource = renderPassResources.mainFrameColorTexture,
                .loadOp = RenderPassOperation::LoadOp::Clear,
                .storeOp = RenderPassOperation::StoreOp::Store
            });
            renderPasses.skyboxPass->AddOutput({
                .resource = renderPassResources.depthMap,
                .loadOp = RenderPassOperation::LoadOp::Clear,
                .storeOp = RenderPassOperation::StoreOp::Store
            });

            renderPasses.skyboxPass->Init();
        }

        // Grid pass
        {
            renderPasses.gridPass->Reset();

            renderPasses.gridPass->AddInput({
                renderPassResources.cameraBuffer
            });

            renderPasses.gridPass->AddOutput({
                .resource = renderPassResources.mainFrameColorTexture,
                .loadOp = RenderPassOperation::LoadOp::Load,
                .storeOp = RenderPassOperation::StoreOp::Store
            });

            renderPasses.gridPass->AddOutput({
                .resource = renderPassResources.depthMap,
                .loadOp = RenderPassOperation::LoadOp::Load,
                .storeOp = RenderPassOperation::StoreOp::Store
            });

            renderPasses.gridPass->Init();
        }

        // GBuffer pass
        {
            renderPasses.gbufferPass->Reset();

            renderPasses.gbufferPass->AddInput({
                renderPassResources.cameraBuffer
            });

            renderPasses.gbufferPass->AddOutput({
                .resource = renderPassResources.viewspaceNormal,
                .loadOp = RenderPassOperation::LoadOp::Clear,
                .storeOp = RenderPassOperation::StoreOp::Store
            });

            renderPasses.gbufferPass->AddOutput({
                .resource = renderPassResources.viewspacePosition,
                .loadOp = RenderPassOperation::LoadOp::Clear,
                .storeOp = RenderPassOperation::StoreOp::Store
            });

            renderPasses.gbufferPass->AddOutput({
                .resource = renderPassResources.depthMap,
                .loadOp = RenderPassOperation::LoadOp::Clear,
                .storeOp = RenderPassOperation::StoreOp::Store
            });

            renderPasses.gbufferPass->Init();
        }

        // Lighting pass
        {
            renderPasses.lightingPass->Reset();

            renderPasses.lightingPass->AddInput(renderPassResources.cameraBuffer);
            renderPasses.lightingPass->AddInput(renderPassResources.lightsBuffer);
            renderPasses.lightingPass->AddInput(renderPassResources.directionalShadowMap);
            renderPasses.lightingPass->AddInput(renderPassResources.irradianceMapTexture);
            renderPasses.lightingPass->AddInput(renderPassResources.ssaoTexture);
            renderPasses.lightingPass->AddInput(renderPassResources.prefilterMapTexture);
            renderPasses.lightingPass->AddInput(renderPassResources.brdfLutTexture);

            renderPasses.lightingPass->AddOutput({
                .resource = renderPassResources.mainFrameColorTexture,
                .loadOp = RenderPassOperation::LoadOp::Load,
                .storeOp = RenderPassOperation::StoreOp::Store
            });

            renderPasses.lightingPass->AddOutput({
                .resource = renderPassResources.depthMap,
                .loadOp = RenderPassOperation::LoadOp::Load,
                .storeOp = RenderPassOperation::StoreOp::Store
            });

            renderPasses.lightingPass->Init();
        }

        CreateFramebuffers();
    }

    void VulkanRenderer::UpdateCameraBuffer(Camera& camera) const
    {
        CameraBuffer bufferData;
        bufferData.cameraPosition = camera.GetTransform().m_Position;
        bufferData.view = camera.GetView();
        bufferData.proj = camera.GetProjection();

        memcpy(renderPassResources.cameraBuffer.resourceInfo.buffer.allocatedBuffer.GetData(), &bufferData, sizeof(CameraBuffer));
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

        memcpy(renderPassResources.lightsBuffer.resourceInfo.buffer.allocatedBuffer.GetData(), &bufferData, sizeof(LightsBuffer));
    }

    void VulkanRenderer::DrawFrame(const VkCommandBuffer commandBuffer, const uint32_t imageIndex, Camera& camera)
    {
        activeImage = imageIndex;

        renderPasses.gbufferPass->Render(commandBuffer, &camera, INVALID_FRAMEBUFFER_HANDLE);
        // TODO Don't forget to add image barriers back later
        //directionalShadowMap->TransitionToDepthRendering(commandBuffer);
        for (size_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
        {
            renderPasses.shadowMapPass->SetCascadeIndex(i);
            renderPasses.shadowMapPass->Render(commandBuffer, &camera, framebuffers.shadowMapFramebuffers[i]);
        }
        //directionalShadowMap->TransitionToShaderRead(commandBuffer);

        renderPasses.ssaoPass->Render(commandBuffer, &camera, framebuffers.ssaoFramebuffer);

        VulkanTexture* depthMap = device->GetTexture(renderPassResources.depthMap.resourceInfo.texture.textureHandle);

        renderPasses.skyboxPass->Render(commandBuffer, &camera, INVALID_FRAMEBUFFER_HANDLE);
        renderPasses.gridPass->Render(commandBuffer, &camera, INVALID_FRAMEBUFFER_HANDLE);

        VulkanUtils::TransitionImageLayout(commandBuffer, depthMap->allocatedImage,
                                           VK_IMAGE_ASPECT_DEPTH_BIT,
                                           VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                           VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);

        renderPasses.lightingPass->Render(commandBuffer, &camera, INVALID_FRAMEBUFFER_HANDLE);

        VulkanUtils::TransitionImageLayout(commandBuffer, depthMap->allocatedImage,
                                           VK_IMAGE_ASPECT_DEPTH_BIT,
                                           VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
                                           VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

        renderPasses.presentPass->Render(commandBuffer, &camera, framebuffers.presentFramebuffers[activeImage]);
    }

    void VulkanRenderer::CreateRenderPassResources()
    {
        // Viewspace Normal
        {
            RenderPassResourceInfo resourceInfo{};
            resourceInfo.texture.textureCreateInfo = {
                .width = renderResolution.width,
                .height = renderResolution.height,
                .format = ImageFormat::RGBA32_SFLOAT,
                .imageLayout = ImageUtils::GetLayoutFromFormat(ImageFormat::RGBA32_SFLOAT),
                .imageInitialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            };

            resourceInfo.texture.textureHandle = device->CreateTexture(resourceInfo.texture.textureCreateInfo);

            renderPassResources.viewspaceNormal = {
                .name = "viewspace_normal",
                .type = ResourceType::Texture,
                .resourceInfo = resourceInfo,
            };

            VulkanTexture* viewspaceNormalTexture = device->GetTexture(
                renderPassResources.viewspaceNormal.resourceInfo.texture.textureHandle);
            const VkDescriptorImageInfo worldSpaceNormalInfo{
                .sampler = viewspaceNormalTexture->sampler,
                .imageView = viewspaceNormalTexture->imageView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };

            auto descriptorSetLayoutHandle = device->GetDescriptorSetLayout(
                ShaderCache::descriptorSetLayouts.viewspaceNormalDescriptorSetLayout);
            VulkanDescriptorWriter(*descriptorSetLayoutHandle, device->GetShaderDescriptorPool())
                    .WriteImage(0, worldSpaceNormalInfo)
                    .BuildOrOverwrite(ShaderCache::descriptorSets.viewspaceNormalDescriptorSet);

            renderPassResourceMap.insert({renderPassResources.viewspaceNormal.name, renderPassResources.viewspaceNormal});
        }

        // Viewspace Position
        {
            RenderPassResourceInfo resourceInfo{};
            resourceInfo.texture.textureCreateInfo = {
                .width = renderResolution.width,
                .height = renderResolution.height,
                .format = ImageFormat::RGBA32_SFLOAT,
                .imageLayout = ImageUtils::GetLayoutFromFormat(ImageFormat::RGBA32_SFLOAT),
                .imageInitialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            };

            resourceInfo.texture.textureHandle = device->CreateTexture(resourceInfo.texture.textureCreateInfo);

            renderPassResources.viewspacePosition = {
                .name = "viewspace_position",
                .type = ResourceType::Texture,
                .resourceInfo = resourceInfo,
            };

            VulkanTexture* viewspacePositionTexture = device->GetTexture(
                renderPassResources.viewspacePosition.resourceInfo.texture.textureHandle);
            const VkDescriptorImageInfo positionInfo{
                .sampler = viewspacePositionTexture->sampler,
                .imageView = viewspacePositionTexture->imageView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };

            auto descriptorSetLayoutHandle = device->GetDescriptorSetLayout(
                ShaderCache::descriptorSetLayouts.viewspacePositionDescriptorSetLayout);
            VulkanDescriptorWriter(*descriptorSetLayoutHandle, device->GetShaderDescriptorPool())
                    .WriteImage(0, positionInfo)
                    .BuildOrOverwrite(ShaderCache::descriptorSets.viewspacePositionDescriptorSet);

            renderPassResourceMap.insert({renderPassResources.viewspacePosition.name, renderPassResources.viewspacePosition});
        }

        // Depth Map
        {
            RenderPassResourceInfo resourceInfo{};
            resourceInfo.texture.textureCreateInfo = {
                .width = renderResolution.width,
                .height = renderResolution.height,
                .format = ImageFormat::DEPTH24_STENCIL8,
                .imageLayout = ImageUtils::GetLayoutFromFormat(ImageFormat::DEPTH24_STENCIL8),
                .imageInitialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                .addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            };

            resourceInfo.texture.textureHandle = device->CreateTexture(resourceInfo.texture.textureCreateInfo);

            renderPassResources.depthMap = {
                .name = "depth_map",
                .type = ResourceType::Texture,
                .resourceInfo = resourceInfo,
            };

            VulkanTexture* depthTexture = device->GetTexture(renderPassResources.depthMap.resourceInfo.texture.textureHandle);

            const VkDescriptorImageInfo depthInfo{
                .sampler = depthTexture->sampler,
                .imageView = depthTexture->imageView,
                .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
            };

            auto descriptorSetLayoutHandle = device->GetDescriptorSetLayout(ShaderCache::descriptorSetLayouts.depthMapDescriptorSetLayout);
            VulkanDescriptorWriter(*descriptorSetLayoutHandle, device->GetShaderDescriptorPool())
                    .WriteImage(0, depthInfo)
                    .BuildOrOverwrite(ShaderCache::descriptorSets.depthMapDescriptorSet);

            renderPassResourceMap.insert({renderPassResources.depthMap.name, renderPassResources.depthMap});
        }

        // SSAO Texture
        {
            RenderPassResourceInfo resourceInfo{};
            resourceInfo.texture.textureCreateInfo = {
                .width = static_cast<uint32_t>(renderResolution.width * 0.5),
                .height = static_cast<uint32_t>(renderResolution.height * 0.5),
                .format = ImageFormat::R8_UNORM,
                .imageLayout = ImageUtils::GetLayoutFromFormat(ImageFormat::R8_UNORM),
            };

            resourceInfo.texture.textureHandle = device->CreateTexture(resourceInfo.texture.textureCreateInfo);

            renderPassResources.ssaoTexture = {
                .name = "ssao_texture",
                .type = ResourceType::Texture,
                .resourceInfo = resourceInfo,
            };

            VulkanTexture* ssaoTexture = device->GetTexture(renderPassResources.ssaoTexture.resourceInfo.texture.textureHandle);
            const VkDescriptorImageInfo ssaoInfo = {
                .sampler = ssaoTexture->sampler,
                .imageView = ssaoTexture->imageView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };

            auto descriptorSetLayoutHandle = device->GetDescriptorSetLayout(
                ShaderCache::descriptorSetLayouts.postProcessingDescriptorSetLayout);
            VulkanDescriptorWriter(*descriptorSetLayoutHandle, device->GetShaderDescriptorPool())
                    .WriteImage(0, ssaoInfo)
                    .BuildOrOverwrite(ShaderCache::descriptorSets.ssaoDescriptorSet);

            renderPassResourceMap.insert({renderPassResources.ssaoTexture.name, renderPassResources.ssaoTexture});
        }

        // Main Frame Color
        {
            RenderPassResourceInfo resourceInfo{};
            resourceInfo.texture.textureCreateInfo = {
                .width = renderResolution.width,
                .height = renderResolution.height,
                .format = ImageFormat::RGBA16_SFLOAT,
                .imageLayout = ImageUtils::GetLayoutFromFormat(ImageFormat::RGBA16_SFLOAT),
                .addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            };

            resourceInfo.texture.textureHandle = device->CreateTexture(resourceInfo.texture.textureCreateInfo);

            renderPassResources.mainFrameColorTexture = {
                .name = "main_frame_color",
                .type = ResourceType::Texture,
                .resourceInfo = resourceInfo,
            };

            VulkanTexture* mainFrameColorTexture = device->GetTexture(
                renderPassResources.mainFrameColorTexture.resourceInfo.texture.textureHandle);

            const VkDescriptorImageInfo renderImageInfo = {
                .sampler = mainFrameColorTexture->sampler,
                .imageView = mainFrameColorTexture->imageView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };

            auto descriptorSetLayoutHandle = device->GetDescriptorSetLayout(ShaderCache::descriptorSetLayouts.presentDescriptorSetLayout);
            VulkanDescriptorWriter(*descriptorSetLayoutHandle, device->GetShaderDescriptorPool())
                    .WriteImage(0, renderImageInfo)
                    .BuildOrOverwrite(shaderCache->descriptorSets.presentDescriptorSet);

            renderPassResourceMap.insert({renderPassResources.mainFrameColorTexture.name, renderPassResources.mainFrameColorTexture});
        }
    }

    void VulkanRenderer::CreateRenderPassBuffers()
    {
        // Lights Buffer
        {
            RenderPassResourceInfo resourceInfo{};
            resourceInfo.buffer.size = sizeof(LightsBuffer);
            resourceInfo.buffer.usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
            resourceInfo.buffer.memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;

            resourceInfo.buffer.allocatedBuffer = {};
            renderPassResources.lightsBuffer = {
                .name = "lights_buffer",
                .type = ResourceType::Buffer,
                .resourceInfo = resourceInfo,
            };

            renderPassResources.lightsBuffer.resourceInfo.buffer.allocatedBuffer = device->CreateBuffer(
                resourceInfo.buffer.size,
                resourceInfo.buffer.usageFlags,
                resourceInfo.buffer.memoryUsage
            );

            VkDescriptorBufferInfo info{};
            info.buffer = renderPassResources.lightsBuffer.resourceInfo.buffer.allocatedBuffer.buffer;
            info.offset = 0;
            info.range = sizeof(LightsBuffer);

            renderPassResourceMap.insert({renderPassResources.lightsBuffer.name, renderPassResources.lightsBuffer});

            VulkanTexture* shadowMapTexture = device->GetTexture(
                renderPassResources.directionalShadowMap.resourceInfo.texture.textureHandle);
            VkDescriptorImageInfo shadowMapInfo = {
                .sampler = shadowMapTexture->GetSampler(),
                .imageView = shadowMapTexture->GetImageView(),
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };

            auto lightsDescriptorSetLayoutHandle = device->GetDescriptorSetLayout(
                ShaderCache::descriptorSetLayouts.lightsDescriptorSetLayout);
            VulkanDescriptorWriter(*lightsDescriptorSetLayoutHandle, device->GetShaderDescriptorPool())
                    .WriteBuffer(0, info)
                    .Build(shaderCache->descriptorSets.lightsDescriptorSet);

            auto shadowmapDescriptorSetLayoutHandle = device->GetDescriptorSetLayout(
                ShaderCache::descriptorSetLayouts.directionalShadowMapDescriptorSetLayout);
            VulkanDescriptorWriter(*shadowmapDescriptorSetLayoutHandle,
                                   device->GetShaderDescriptorPool())
                    .WriteImage(0, shadowMapInfo)
                    .Build(shaderCache->descriptorSets.directionalShadownMapDescriptorSet);
        }

        // Camera Buffer
        {
            RenderPassResourceInfo resourceInfo{};
            resourceInfo.buffer.size = sizeof(CameraBuffer);
            resourceInfo.buffer.usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
            resourceInfo.buffer.memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;

            resourceInfo.buffer.allocatedBuffer = {};
            renderPassResources.cameraBuffer = {
                .name = "camera_buffer",
                .type = ResourceType::Buffer,
                .resourceInfo = resourceInfo,
            };

            renderPassResources.cameraBuffer.resourceInfo.buffer.allocatedBuffer = device->CreateBuffer(
                resourceInfo.buffer.size,
                resourceInfo.buffer.usageFlags,
                resourceInfo.buffer.memoryUsage
            );

            VkDescriptorBufferInfo info{};
            info.buffer = renderPassResources.cameraBuffer.resourceInfo.buffer.allocatedBuffer.buffer;
            info.offset = 0;
            info.range = sizeof(CameraBuffer);

            auto descriptorSetLayoutHandle = device->GetDescriptorSetLayout(ShaderCache::descriptorSetLayouts.cameraDescriptorSetLayout);
            VulkanDescriptorWriter(*descriptorSetLayoutHandle, device->GetShaderDescriptorPool())
                    .WriteBuffer(0, info)
                    .Build(shaderCache->descriptorSets.cameraDescriptorSet);

            renderPassResourceMap.insert({renderPassResources.cameraBuffer.name, renderPassResources.cameraBuffer});
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

            RenderPassResourceInfo resourceInfo{};
            resourceInfo.texture.textureHandle = device->CreateTexture(textureCreateInfo);
            device->UploadCubemapTextureData(resourceInfo.texture.textureHandle, &cubemapBitmap);

            renderPassResources.skyboxTexture = {
                .name = "skybox_texture",
                .type = ResourceType::TextureCube,
                .resourceInfo = resourceInfo,
            };

            VulkanTexture* skyboxTexture = device->GetTexture(renderPassResources.skyboxTexture.resourceInfo.texture.textureHandle);

            VkDescriptorImageInfo info{
                skyboxTexture->GetSampler(),
                skyboxTexture->GetImageView(),
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            };

            auto descriptorSetLayoutHandle = device->GetDescriptorSetLayout(ShaderCache::descriptorSetLayouts.cubemapDescriptorSetLayout);
            VulkanDescriptorWriter(*descriptorSetLayoutHandle, device->GetShaderDescriptorPool())
                    .WriteImage(0, info)
                    .Build(ShaderCache::descriptorSets.cubemapDescriptorSet);

            renderPassResourceMap.insert({renderPassResources.skyboxTexture.name, renderPassResources.skyboxTexture});
        }

        // BRDF LUT
        {
            RenderPassResourceInfo resourceInfo{};
            resourceInfo.texture.textureCreateInfo = {};
            resourceInfo.texture.textureCreateInfo.width = 512;
            resourceInfo.texture.textureCreateInfo.height = 512;
            resourceInfo.texture.textureCreateInfo.format = ImageFormat::RGBA16_SFLOAT;
            resourceInfo.texture.textureCreateInfo.imageLayout = ImageUtils::GetLayoutFromFormat(ImageFormat::RGBA16_SFLOAT);

            resourceInfo.texture.textureHandle = device->CreateTexture(resourceInfo.texture.textureCreateInfo);

            renderPassResources.brdfLutTexture = {
                .name = "brdflut_texture",
                .type = ResourceType::Texture,
                .resourceInfo = resourceInfo,
            };

            VulkanTexture* brdfLUTTexture = device->GetTexture(renderPassResources.brdfLutTexture.resourceInfo.texture.textureHandle);
            VkDescriptorImageInfo brdfLUTInfo{};
            brdfLUTInfo.sampler = brdfLUTTexture->GetSampler();
            brdfLUTInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            brdfLUTInfo.imageView = brdfLUTTexture->GetImageView();

            auto descriptorSetLayoutHandle = device->GetDescriptorSetLayout(ShaderCache::descriptorSetLayouts.brdfLutDescriptorSetLayout);
            VulkanDescriptorWriter(*descriptorSetLayoutHandle, device->GetShaderDescriptorPool())
                    .WriteImage(0, brdfLUTInfo)
                    .Build(ShaderCache::descriptorSets.brdfLutDescriptorSet);

            renderPassResourceMap.insert({renderPassResources.brdfLutTexture.name, renderPassResources.brdfLutTexture});
        }

        // Prefilter Map
        {
            constexpr uint32_t PREFILTER_MIP_LEVELS = 6;
            constexpr uint32_t REFLECTION_RESOLUTION = 256;

            RenderPassResourceInfo resourceInfo{};
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

            resourceInfo.texture.textureHandle = device->CreateTexture(textureCreateInfo);

            renderPassResources.prefilterMapTexture = {
                .name = "prefilter_map_texture",
                .type = ResourceType::TextureCube,
                .resourceInfo = resourceInfo,
            };

            VulkanTexture* prefilterMapTexture = device->GetTexture(
                renderPassResources.prefilterMapTexture.resourceInfo.texture.textureHandle);
            VkDescriptorImageInfo prefilterMapInfo{};
            prefilterMapInfo.sampler = prefilterMapTexture->GetSampler();
            prefilterMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            prefilterMapInfo.imageView = prefilterMapTexture->GetImageView();

            auto descriptorSetLayoutHandle = device->
                    GetDescriptorSetLayout(ShaderCache::descriptorSetLayouts.reflectionDescriptorSetLayout);
            VulkanDescriptorWriter(*descriptorSetLayoutHandle, device->GetShaderDescriptorPool())
                    .WriteImage(0, prefilterMapInfo)
                    .Build(ShaderCache::descriptorSets.reflectionDescriptorSet);

            renderPassResourceMap.insert({renderPassResources.prefilterMapTexture.name, renderPassResources.prefilterMapTexture});
        }

        // Irradiance Map
        {
            RenderPassResourceInfo resourceInfo{};
            resourceInfo.texture.textureCreateInfo = {};
            resourceInfo.texture.textureCreateInfo.width = 32;
            resourceInfo.texture.textureCreateInfo.height = 32;
            resourceInfo.texture.textureCreateInfo.format = ImageFormat::RGBA16_SFLOAT;
            resourceInfo.texture.textureCreateInfo.imageLayout = ImageUtils::GetLayoutFromFormat(ImageFormat::RGBA16_SFLOAT);
            resourceInfo.texture.textureCreateInfo.addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            resourceInfo.texture.textureCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
            resourceInfo.texture.textureCreateInfo.isCubeMap = true;
            resourceInfo.texture.textureCreateInfo.arrayLayers = 6;

            resourceInfo.texture.textureHandle = device->CreateTexture(resourceInfo.texture.textureCreateInfo);

            renderPassResources.irradianceMapTexture = {
                .name = "irradiance_map_texture",
                .type = ResourceType::TextureCube,
                .resourceInfo = resourceInfo,
            };

            renderPassResourceMap.insert({renderPassResources.irradianceMapTexture.name, renderPassResources.irradianceMapTexture});
        }

        // Directional Shadow Map
        {
            const uint16_t SHADOW_MAP_RESOLUTION = EnumValue(scene.directionalLight.shadowMapResolution);

            RenderPassResourceInfo resourceInfo{};
            resourceInfo.texture.textureCreateInfo = {
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

            resourceInfo.texture.textureHandle = device->CreateTexture(resourceInfo.texture.textureCreateInfo);

            renderPassResources.directionalShadowMap = {
                .name = "directional_shadow_map",
                .type = ResourceType::Texture,
                .resourceInfo = resourceInfo,
            };

            renderPassResourceMap.insert({renderPassResources.directionalShadowMap.name, renderPassResources.directionalShadowMap});
        }
    }
}
