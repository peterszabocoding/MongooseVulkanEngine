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
        device->DestroyBuffer(renderPassResourceMap["camera_buffer"].resourceInfo.buffer.allocatedBuffer);
        device->DestroyBuffer(renderPassResourceMap["lights_buffer"].resourceInfo.buffer.allocatedBuffer);
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

        presentDescriptorSetLayout = VulkanDescriptorSetLayoutBuilder(device)
                .AddBinding({0, DescriptorSetBindingType::TextureSampler, {ShaderStage::FragmentShader}})
                .Build();
    }

    void VulkanRenderer::InitializeRenderPasses()
    {
        // Skybox pass
        {
            renderPasses.skyboxPass->Reset();

            renderPasses.skyboxPass->AddInput(renderPassResourceMap["skybox_texture"]);
            renderPasses.skyboxPass->AddInput(renderPassResourceMap["camera_buffer"]);

            renderPasses.skyboxPass->AddOutput({
                .resource = renderPassResourceMap["main_frame_color"],
                .loadOp = RenderPassOperation::LoadOp::Clear,
                .storeOp = RenderPassOperation::StoreOp::Store
            });
            renderPasses.skyboxPass->AddOutput({
                .resource = renderPassResourceMap["depth_map"],
                .loadOp = RenderPassOperation::LoadOp::Clear,
                .storeOp = RenderPassOperation::StoreOp::Store
            });

            renderPasses.skyboxPass->Init();
        }

        // Grid pass
        {
            renderPasses.gridPass->Reset();

            renderPasses.gridPass->AddInput(renderPassResourceMap["camera_buffer"]);

            renderPasses.gridPass->AddOutput({
                .resource = renderPassResourceMap["main_frame_color"],
                .loadOp = RenderPassOperation::LoadOp::Load,
                .storeOp = RenderPassOperation::StoreOp::Store
            });

            renderPasses.gridPass->AddOutput({
                .resource = renderPassResourceMap["depth_map"],
                .loadOp = RenderPassOperation::LoadOp::Load,
                .storeOp = RenderPassOperation::StoreOp::Store
            });

            renderPasses.gridPass->Init();
        }

        // BRDF LUT pass
        {
            renderPasses.brdfLutPass->Reset();

            renderPasses.brdfLutPass->AddOutput({
                .resource = renderPassResourceMap["brdflut_texture"],
                .loadOp = RenderPassOperation::LoadOp::Clear,
                .storeOp = RenderPassOperation::StoreOp::Store
            });

            renderPasses.brdfLutPass->Init();
        }

        // GBuffer pass
        {
            renderPasses.gbufferPass->Reset();

            renderPasses.gbufferPass->AddInput(renderPassResourceMap["camera_buffer"]);

            renderPasses.gbufferPass->AddOutput({
                .resource = renderPassResourceMap["viewspace_normal"],
                .loadOp = RenderPassOperation::LoadOp::Clear,
                .storeOp = RenderPassOperation::StoreOp::Store
            });

            renderPasses.gbufferPass->AddOutput({
                .resource = renderPassResourceMap["viewspace_position"],
                .loadOp = RenderPassOperation::LoadOp::Clear,
                .storeOp = RenderPassOperation::StoreOp::Store
            });

            renderPasses.gbufferPass->AddOutput({
                .resource = renderPassResourceMap["depth_map"],
                .loadOp = RenderPassOperation::LoadOp::Clear,
                .storeOp = RenderPassOperation::StoreOp::Store
            });

            renderPasses.gbufferPass->Init();
        }

        // Lighting pass
        {
            renderPasses.lightingPass->Reset();

            renderPasses.lightingPass->AddInput(renderPassResourceMap["camera_buffer"]);
            renderPasses.lightingPass->AddInput(renderPassResourceMap["lights_buffer"]);
            renderPasses.lightingPass->AddInput(renderPassResourceMap["directional_shadow_map"]);
            renderPasses.lightingPass->AddInput(renderPassResourceMap["irradiance_map_texture"]);
            renderPasses.lightingPass->AddInput(renderPassResourceMap["ssao_texture"]);
            renderPasses.lightingPass->AddInput(renderPassResourceMap["prefilter_map_texture"]);
            renderPasses.lightingPass->AddInput(renderPassResourceMap["brdflut_texture"]);

            renderPasses.lightingPass->AddOutput({
                .resource = renderPassResourceMap["main_frame_color"],
                .loadOp = RenderPassOperation::LoadOp::Load,
                .storeOp = RenderPassOperation::StoreOp::Store
            });

            renderPasses.lightingPass->AddOutput({
                .resource = renderPassResourceMap["depth_map"],
                .loadOp = RenderPassOperation::LoadOp::Load,
                .storeOp = RenderPassOperation::StoreOp::Store
            });

            renderPasses.lightingPass->Init();
        }

        // SSAO pass
        {
            renderPasses.ssaoPass->Reset();

            renderPasses.ssaoPass->AddInput(renderPassResourceMap["viewspace_normal"]);
            renderPasses.ssaoPass->AddInput(renderPassResourceMap["viewspace_position"]);
            renderPasses.ssaoPass->AddInput(renderPassResourceMap["depth_map"]);
            renderPasses.ssaoPass->AddInput(renderPassResourceMap["camera_buffer"]);

            renderPasses.ssaoPass->AddOutput({
                .resource = renderPassResourceMap["ssao_texture"],
                .loadOp = RenderPassOperation::LoadOp::Clear,
                .storeOp = RenderPassOperation::StoreOp::Store
            });

            renderPasses.ssaoPass->Init();
        }

        // Shadow map pass
        {
            renderPasses.shadowMapPass->Reset();

            renderPasses.shadowMapPass->AddOutput({
                .resource = renderPassResourceMap["directional_shadow_map"],
                .loadOp = RenderPassOperation::LoadOp::Clear,
                .storeOp = RenderPassOperation::StoreOp::Store
            });

            renderPasses.shadowMapPass->Init();
        }

        // Prefilter map pass
        {
            renderPasses.prefilterMapPass->Reset();

            renderPasses.prefilterMapPass->AddInput(renderPassResourceMap["skybox_texture"]);

            renderPasses.prefilterMapPass->AddOutput({
                .resource = renderPassResourceMap["prefilter_map_texture"],
                .loadOp = RenderPassOperation::LoadOp::Clear,
                .storeOp = RenderPassOperation::StoreOp::Store
            });

            renderPasses.prefilterMapPass->Init();
        }

        // Irradiance map pass
        {
            renderPasses.irradianceMapPass->Reset();

            renderPasses.irradianceMapPass->AddInput(renderPassResourceMap["skybox_texture"]);

            renderPasses.irradianceMapPass->AddOutput({
                .resource = renderPassResourceMap["irradiance_map_texture"],
                .loadOp = RenderPassOperation::LoadOp::Clear,
                .storeOp = RenderPassOperation::StoreOp::Store
            });

            renderPasses.irradianceMapPass->Init();
        }

        // UI pass
        {
            renderPasses.uiPass->Reset();
            renderPasses.uiPass->AddOutput({
                .resource = renderPassResourceMap["main_frame_color"],
                .loadOp = RenderPassOperation::LoadOp::Load,
                .storeOp = RenderPassOperation::StoreOp::Store
            });
            renderPasses.uiPass->Init();
        }
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

        const uint16_t SHADOW_MAP_RESOLUTION = EnumValue(scene.directionalLight.shadowMapResolution);
        const VkExtent2D SSAO_RESOLUTION = {
            static_cast<uint32_t>(renderResolution.width * 0.5),
            static_cast<uint32_t>(renderResolution.height * 0.5)
        };

        //
        renderPasses.gbufferPass        =    CreateScope<GBufferPass>(device, scene, renderResolution);
        renderPasses.lightingPass       =    CreateScope<LightingPass>(device, scene, renderResolution);
        renderPasses.shadowMapPass      =    CreateScope<ShadowMapPass>(device, scene, VkExtent2D{SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION});
        renderPasses.ssaoPass           =    CreateScope<SSAOPass>(device, SSAO_RESOLUTION);
        renderPasses.irradianceMapPass  =    CreateScope<IrradianceMapPass>(device, renderResolution);
        renderPasses.brdfLutPass        =    CreateScope<BrdfLUTPass>(device, VkExtent2D{512, 512});
        renderPasses.prefilterMapPass   =    CreateScope<PrefilterMapPass>(device, renderResolution);
        renderPasses.skyboxPass         =    CreateScope<SkyboxPass>(device, scene, renderResolution);
        renderPasses.gridPass           =    CreateScope<InfiniteGridPass>(device, renderResolution);
        renderPasses.uiPass             =    CreateScope<UiPass>(device, renderResolution);

        InitializeRenderPasses();

        // IBL and reflection calculations
        device->ImmediateSubmit([&](VkCommandBuffer commandBuffer) {
            renderPasses.irradianceMapPass->Render(commandBuffer);
            renderPasses.brdfLutPass->Render(commandBuffer);
            renderPasses.prefilterMapPass->Render(commandBuffer);
        });

        isSceneLoaded = true;
    }

    void VulkanRenderer::Draw(const float deltaTime, Camera& camera)
    {
        if (!isSceneLoaded) return;

        RotateLight(deltaTime);
        scene.directionalLight.UpdateCascades(camera);

        UpdateLightsBuffer();
        UpdateCameraBuffer(camera);

        device->DrawFrame(vulkanSwapChain->GetSwapChain(),
                          [&](VkCommandBuffer cmd, uint32_t imgIndex) { DrawFrame(cmd, imgIndex); },
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

    void VulkanRenderer::ResizeSwapchain()
    {
        IdleWait();

        CreateSwapchain();
        CreateRenderPassResources();

        renderPasses.gbufferPass->Resize(renderResolution);
        renderPasses.skyboxPass->Resize(renderResolution);
        renderPasses.lightingPass->Resize(renderResolution);
        renderPasses.ssaoPass->Resize(VkExtent2D{
            static_cast<uint32_t>(renderResolution.width * 0.5),
            static_cast<uint32_t>(renderResolution.height * 0.5)
        });
        renderPasses.gridPass->Resize(renderResolution);
        renderPasses.uiPass->Resize(renderResolution);

        InitializeRenderPasses();
    }

    void VulkanRenderer::UpdateCameraBuffer(Camera& camera)
    {
        const CameraBuffer bufferData{
            .proj = camera.GetProjection(),
            .view = camera.GetView(),
            .cameraPosition = camera.GetTransform().m_Position
        };

        memcpy(renderPassResourceMap["camera_buffer"].resourceInfo.buffer.allocatedBuffer.GetData(), &bufferData, sizeof(CameraBuffer));
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
            bufferData.cascadeSplits[i].x = scene.directionalLight.cascades[i].splitDepth;
        }

        bufferData.direction = glm::vec4(normalize(scene.directionalLight.direction), 0.0);
        bufferData.color = glm::vec4(scene.directionalLight.color, 1.0f);
        bufferData.intensity = scene.directionalLight.intensity;
        bufferData.ambientIntensity = scene.directionalLight.ambientIntensity;
        bufferData.bias = scene.directionalLight.bias;

        memcpy(renderPassResourceMap["lights_buffer"].resourceInfo.buffer.allocatedBuffer.GetData(), &bufferData, sizeof(LightsBuffer));
    }

    void VulkanRenderer::DrawFrame(VkCommandBuffer commandBuffer, uint32_t imageIndex)
    {
        activeImage = imageIndex;

        if (vulkanSwapChain->GetExtent().width != renderResolution.width || vulkanSwapChain->GetExtent().height != renderResolution.height)
            return;

        renderPasses.gbufferPass->Render(commandBuffer);
        renderPasses.shadowMapPass->Render(commandBuffer);
        renderPasses.ssaoPass->Render(commandBuffer);
        renderPasses.skyboxPass->Render(commandBuffer);
        renderPasses.gridPass->Render(commandBuffer);

        // Lighting pass
        {
            VulkanTexture* depthMap = device->GetTexture(renderPassResourceMap["depth_map"].resourceInfo.texture.textureHandle);
            VulkanUtils::TransitionImageLayout(commandBuffer, depthMap->allocatedImage,
                                               VK_IMAGE_ASPECT_DEPTH_BIT,
                                               VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                               VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);

            renderPasses.lightingPass->Render(commandBuffer);

            VulkanUtils::TransitionImageLayout(commandBuffer, depthMap->allocatedImage,
                                               VK_IMAGE_ASPECT_DEPTH_BIT,
                                               VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
                                               VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        }

        renderPasses.uiPass->Render(commandBuffer);

        // Present
        {
            VkImage swapchainImage = vulkanSwapChain->GetImages()[activeImage];
            VulkanTexture* mainFrameTexture = device->GetTexture(
                renderPassResourceMap["main_frame_color"].resourceInfo.texture.textureHandle);
            VulkanUtils::TransitionImageLayout(commandBuffer, mainFrameTexture->allocatedImage,
                                               VK_IMAGE_ASPECT_COLOR_BIT,
                                               VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

            VulkanUtils::TransitionImageLayout(commandBuffer, swapchainImage,
                                               VK_IMAGE_ASPECT_COLOR_BIT,
                                               VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            VulkanUtils::CopyParams copyParams{};
            copyParams.regionWidth = renderResolution.width;
            copyParams.regionHeight = renderResolution.height;

            CopyImage(commandBuffer, mainFrameTexture->allocatedImage, swapchainImage, copyParams);

            VulkanUtils::TransitionImageLayout(commandBuffer, swapchainImage,
                                               VK_IMAGE_ASPECT_COLOR_BIT,
                                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                               VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

            VulkanUtils::TransitionImageLayout(commandBuffer, mainFrameTexture->allocatedImage,
                                               VK_IMAGE_ASPECT_COLOR_BIT,
                                               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                               VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        }
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

            PassResource passResource = {
                .name = "viewspace_normal",
                .type = ResourceType::Texture,
                .resourceInfo = resourceInfo,
            };

            renderPassResourceMap[passResource.name] = passResource;
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

            PassResource passResource = {
                .name = "viewspace_position",
                .type = ResourceType::Texture,
                .resourceInfo = resourceInfo,
            };

            renderPassResourceMap[passResource.name] = passResource;
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

            PassResource passResource = {
                .name = "depth_map",
                .type = ResourceType::Texture,
                .resourceInfo = resourceInfo,
            };

            renderPassResourceMap[passResource.name] = passResource;
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

            PassResource passResource = {
                .name = "ssao_texture",
                .type = ResourceType::Texture,
                .resourceInfo = resourceInfo,
            };

            renderPassResourceMap[passResource.name] = passResource;
        }

        // Main Frame Color
        {
            RenderPassResourceInfo resourceInfo{};
            resourceInfo.texture.textureCreateInfo = {
                .width = renderResolution.width,
                .height = renderResolution.height,
                .format = ImageFormat::RGBA8_UNORM,
                .imageLayout = ImageUtils::GetLayoutFromFormat(ImageFormat::RGBA8_UNORM),
                .imageInitialLayout = ImageUtils::GetLayoutFromFormat(ImageFormat::RGBA8_UNORM),
                .addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            };

            resourceInfo.texture.textureHandle = device->CreateTexture(resourceInfo.texture.textureCreateInfo);

            PassResource passResource = {
                .name = "main_frame_color",
                .type = ResourceType::Texture,
                .resourceInfo = resourceInfo,
            };

            renderPassResourceMap[passResource.name] = passResource;

            VulkanTexture* mainFrameColorTexture = device->GetTexture(passResource.resourceInfo.texture.textureHandle);

            const VkDescriptorImageInfo renderImageInfo = {
                .sampler = mainFrameColorTexture->sampler,
                .imageView = mainFrameColorTexture->imageView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };

            auto descriptorSetLayoutHandle = device->GetDescriptorSetLayout(presentDescriptorSetLayout);
            VulkanDescriptorWriter(*descriptorSetLayoutHandle, device->GetShaderDescriptorPool())
                    .WriteImage(0, renderImageInfo)
                    .BuildOrOverwrite(presentDescriptorSet);
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
            resourceInfo.buffer.allocatedBuffer = device->CreateBuffer(
                resourceInfo.buffer.size,
                resourceInfo.buffer.usageFlags,
                resourceInfo.buffer.memoryUsage
            );

            PassResource passResource = {
                .name = "lights_buffer",
                .type = ResourceType::Buffer,
                .resourceInfo = resourceInfo,
            };

            renderPassResourceMap[passResource.name] = passResource;
        }

        // Camera Buffer
        {
            RenderPassResourceInfo resourceInfo{};
            resourceInfo.buffer.size = sizeof(CameraBuffer);
            resourceInfo.buffer.usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
            resourceInfo.buffer.memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;
            resourceInfo.buffer.allocatedBuffer = device->CreateBuffer(
                resourceInfo.buffer.size,
                resourceInfo.buffer.usageFlags,
                resourceInfo.buffer.memoryUsage
            );

            PassResource passResource = {
                .name = "camera_buffer",
                .type = ResourceType::Buffer,
                .resourceInfo = resourceInfo,
            };

            renderPassResourceMap[passResource.name] = passResource;
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
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                .arrayLayers = 6,
                .flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
                .isCubeMap = true,
            };

            RenderPassResourceInfo resourceInfo{};
            resourceInfo.texture.textureHandle = device->CreateTexture(textureCreateInfo);
            device->UploadCubemapTextureData(resourceInfo.texture.textureHandle, &cubemapBitmap);

            PassResource passResource = {
                .name = "skybox_texture",
                .type = ResourceType::TextureCube,
                .resourceInfo = resourceInfo,
            };

            renderPassResourceMap[passResource.name] = passResource;
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

            PassResource passResource = {
                .name = "brdflut_texture",
                .type = ResourceType::Texture,
                .resourceInfo = resourceInfo,
            };

            renderPassResourceMap[passResource.name] = passResource;
        }

        // Prefilter Map
        {
            constexpr uint32_t PREFILTER_MIP_LEVELS = 6;
            constexpr uint32_t REFLECTION_RESOLUTION = 256;

            RenderPassResourceInfo resourceInfo{};
            resourceInfo.texture.textureCreateInfo.width = REFLECTION_RESOLUTION;
            resourceInfo.texture.textureCreateInfo.height = REFLECTION_RESOLUTION;
            resourceInfo.texture.textureCreateInfo.format = ImageFormat::RGBA16_SFLOAT;
            resourceInfo.texture.textureCreateInfo.imageLayout = ImageUtils::GetLayoutFromFormat(ImageFormat::RGBA16_SFLOAT);
            resourceInfo.texture.textureCreateInfo.addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            resourceInfo.texture.textureCreateInfo.mipLevels = PREFILTER_MIP_LEVELS;
            resourceInfo.texture.textureCreateInfo.arrayLayers = 6;
            resourceInfo.texture.textureCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
            resourceInfo.texture.textureCreateInfo.isCubeMap = true;

            resourceInfo.texture.textureHandle = device->CreateTexture(resourceInfo.texture.textureCreateInfo);

            PassResource passResource;
            passResource = {
                .name = "prefilter_map_texture",
                .type = ResourceType::TextureCube,
                .resourceInfo = resourceInfo,
            };

            renderPassResourceMap[passResource.name] = passResource;
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

            PassResource passResource = {
                .name = "irradiance_map_texture",
                .type = ResourceType::TextureCube,
                .resourceInfo = resourceInfo,
            };

            renderPassResourceMap[passResource.name] = passResource;
        }

        // Directional Shadow Map
        {
            const uint16_t SHADOW_MAP_RESOLUTION = EnumValue(scene.directionalLight.shadowMapResolution);

            RenderPassResourceInfo resourceInfo{};
            resourceInfo.texture.textureCreateInfo = {
                .width = SHADOW_MAP_RESOLUTION,
                .height = SHADOW_MAP_RESOLUTION,
                .format = ImageFormat::DEPTH32,
                .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                .addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
                .arrayLayers = SHADOW_MAP_CASCADE_COUNT,
                .compareEnabled = true,
                .compareOp = VK_COMPARE_OP_LESS
            };

            resourceInfo.texture.textureHandle = device->CreateTexture(resourceInfo.texture.textureCreateInfo);

            PassResource passResource = {
                .name = "directional_shadow_map",
                .type = ResourceType::Texture,
                .resourceInfo = resourceInfo,
            };

            renderPassResourceMap[passResource.name] = passResource;
        }
    }
}
