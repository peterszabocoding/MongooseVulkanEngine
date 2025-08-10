#include "renderer/vulkan/vulkan_renderer.h"

#include <ranges>
#include <renderer/vulkan/vulkan_texture.h>
#include <renderer/vulkan/vulkan_utils.h>
#include <renderer/vulkan/pass/lighting_pass.h>
#include <renderer/vulkan/pass/lighting/brdf_lut_pass.h>

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

        frameGraphResources.Init(128);

        CreateSwapchain();
    }

    void VulkanRenderer::InitializeRenderPasses()
    {
        // Skybox pass
        {
            renderPasses.skyboxPass->Reset();

            renderPasses.skyboxPass->AddInput(renderPassResourceMap["camera_buffer"]);

            renderPasses.skyboxPass->AddOutput({
                .resource = renderPassResourceMap["hdr_image"],
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
                .resource = renderPassResourceMap["hdr_image"],
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
                .resource = renderPassResourceMap["hdr_image"],
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

            renderPasses.prefilterMapPass->AddOutput({
                .resource = renderPassResourceMap["prefilter_map_texture"],
                .loadOp = RenderPassOperation::LoadOp::Clear,
                .storeOp = RenderPassOperation::StoreOp::Store
            });

            renderPasses.prefilterMapPass->SetCubemapTexture(scene.skyboxTexture);

            renderPasses.prefilterMapPass->Init();
        }

        // Irradiance map pass
        {
            renderPasses.irradianceMapPass->Reset();

            renderPasses.irradianceMapPass->AddOutput({
                .resource = renderPassResourceMap["irradiance_map_texture"],
                .loadOp = RenderPassOperation::LoadOp::Clear,
                .storeOp = RenderPassOperation::StoreOp::Store
            });

            renderPasses.irradianceMapPass->SetCubemapTexture(scene.skyboxTexture);

            renderPasses.irradianceMapPass->Init();
        }

        // Tone Mapping pass
        {
            renderPasses.toneMappingPass->Reset();

            renderPasses.toneMappingPass->AddInput(renderPassResourceMap["hdr_image"]);

            renderPasses.toneMappingPass->AddOutput({
                .resource = renderPassResourceMap["main_frame_color"],
                .loadOp = RenderPassOperation::LoadOp::Clear,
                .storeOp = RenderPassOperation::StoreOp::Store
            });
            renderPasses.toneMappingPass->Init();
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

        CreateFrameGraphInputs();
        CreateFrameGraphOutputs();

        renderPasses.gbufferPass = CreateScope<GBufferPass>(device, renderResolution);
        renderPasses.lightingPass = CreateScope<LightingPass>(device, renderResolution);
        renderPasses.shadowMapPass = CreateScope<ShadowMapPass>(device, renderResolution);

        renderPasses.ssaoPass = CreateScope<SSAOPass>(device, renderResolution);
        renderPasses.toneMappingPass = CreateScope<ToneMappingPass>(device, renderResolution);

        renderPasses.irradianceMapPass = CreateScope<IrradianceMapPass>(device, renderResolution);
        renderPasses.brdfLutPass = CreateScope<BrdfLUTPass>(device, renderResolution);
        renderPasses.prefilterMapPass = CreateScope<PrefilterMapPass>(device, renderResolution);

        renderPasses.skyboxPass = CreateScope<SkyboxPass>(device, renderResolution);
        renderPasses.gridPass = CreateScope<InfiniteGridPass>(device, renderResolution);
        renderPasses.uiPass = CreateScope<UiPass>(device, renderResolution);

        InitializeRenderPasses();

        // IBL and reflection calculations
        device->ImmediateSubmit([&](VkCommandBuffer commandBuffer) {
            renderPasses.irradianceMapPass->Render(commandBuffer, &scene);
            renderPasses.brdfLutPass->Render(commandBuffer, &scene);
            renderPasses.prefilterMapPass->Render(commandBuffer, &scene);
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
        CreateFrameGraphOutputs();

        renderPasses.gbufferPass->Resize(renderResolution);
        renderPasses.skyboxPass->Resize(renderResolution);
        renderPasses.lightingPass->Resize(renderResolution);
        renderPasses.ssaoPass->Resize(renderResolution);
        renderPasses.toneMappingPass->Resize(renderResolution);
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

        renderPasses.gbufferPass->Render(commandBuffer, &scene);
        renderPasses.shadowMapPass->Render(commandBuffer, &scene);
        renderPasses.ssaoPass->Render(commandBuffer, &scene);
        renderPasses.skyboxPass->Render(commandBuffer, &scene);
        renderPasses.gridPass->Render(commandBuffer, &scene);

        // Lighting pass
        {
            VulkanTexture* depthMap = device->GetTexture(renderPassResourceMap["depth_map"].resourceInfo.texture.textureHandle);
            VulkanUtils::TransitionImageLayout(commandBuffer, depthMap->allocatedImage,
                                               VK_IMAGE_ASPECT_DEPTH_BIT,
                                               VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                               VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);

            renderPasses.lightingPass->Render(commandBuffer, &scene);

            VulkanUtils::TransitionImageLayout(commandBuffer, depthMap->allocatedImage,
                                               VK_IMAGE_ASPECT_DEPTH_BIT,
                                               VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
                                               VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        }

        renderPasses.toneMappingPass->Render(commandBuffer, &scene);

        renderPasses.uiPass->Render(commandBuffer, &scene);

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

    void VulkanRenderer::CreateFrameGraphOutputs()
    {
        // Viewspace Normal
        {
            FrameGraphResourceOutputCreation outputCreation{};
            outputCreation.name = "viewspace_normal";
            outputCreation.type = FrameGraphResourceType::Texture;
            outputCreation.resourceInfo.texture.textureCreateInfo = {
                .width = renderResolution.width,
                .height = renderResolution.height,
                .format = ImageFormat::RGBA32_SFLOAT,
                .imageLayout = ImageUtils::GetLayoutFromFormat(ImageFormat::RGBA32_SFLOAT),
                .imageInitialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            };

            frameGraphOutputCreations[outputCreation.name] = outputCreation;
        }

        // Viewspace Position
        {
            FrameGraphResourceOutputCreation outputCreation{};
            outputCreation.name = "viewspace_position";
            outputCreation.type = FrameGraphResourceType::Texture;
            outputCreation.resourceInfo.texture.textureCreateInfo = {
                .width = renderResolution.width,
                .height = renderResolution.height,
                .format = ImageFormat::RGBA32_SFLOAT,
                .imageLayout = ImageUtils::GetLayoutFromFormat(ImageFormat::RGBA32_SFLOAT),
                .imageInitialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            };

            frameGraphOutputCreations[outputCreation.name] = outputCreation;
        }

        // Depth Map
        {
            FrameGraphResourceOutputCreation outputCreation{};
            outputCreation.name = "depth_map";
            outputCreation.type = FrameGraphResourceType::Texture;
            outputCreation.resourceInfo.texture.textureCreateInfo = {
                .width = renderResolution.width,
                .height = renderResolution.height,
                .format = ImageFormat::DEPTH24_STENCIL8,
                .imageLayout = ImageUtils::GetLayoutFromFormat(ImageFormat::DEPTH24_STENCIL8),
                .imageInitialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                .addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            };

            frameGraphOutputCreations[outputCreation.name] = outputCreation;
        }

        // SSAO Texture
        {
            FrameGraphResourceOutputCreation outputCreation{};
            outputCreation.name = "ssao_texture";
            outputCreation.type = FrameGraphResourceType::Texture;
            outputCreation.resourceInfo.texture.textureCreateInfo = {
                .width = static_cast<uint32_t>(renderResolution.width * 0.5),
                .height = static_cast<uint32_t>(renderResolution.height * 0.5),
                .format = ImageFormat::R8_UNORM,
                .imageLayout = ImageUtils::GetLayoutFromFormat(ImageFormat::R8_UNORM),
            };

            frameGraphOutputCreations[outputCreation.name] = outputCreation;
        }

        // HDR Image
        {
            FrameGraphResourceOutputCreation outputCreation{};
            outputCreation.name = "hdr_image";
            outputCreation.type = FrameGraphResourceType::Texture;
            outputCreation.resourceInfo.texture.textureCreateInfo = {
                .width = renderResolution.width,
                .height = renderResolution.height,
                .format = ImageFormat::RGBA16_SFLOAT,
                .imageLayout = ImageUtils::GetLayoutFromFormat(ImageFormat::RGBA16_SFLOAT),
                .imageInitialLayout = ImageUtils::GetLayoutFromFormat(ImageFormat::RGBA16_SFLOAT),
                .addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            };

            frameGraphOutputCreations[outputCreation.name] = outputCreation;
        }

        // Main Frame Color
        {
            FrameGraphResourceOutputCreation outputCreation{};
            outputCreation.name = "main_frame_color";
            outputCreation.type = FrameGraphResourceType::Texture;
            outputCreation.resourceInfo.texture.textureCreateInfo = {
                .width = renderResolution.width,
                .height = renderResolution.height,
                .format = ImageFormat::RGBA8_UNORM,
                .imageLayout = ImageUtils::GetLayoutFromFormat(ImageFormat::RGBA8_UNORM),
                .imageInitialLayout = ImageUtils::GetLayoutFromFormat(ImageFormat::RGBA8_UNORM),
                .addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            };

            frameGraphOutputCreations[outputCreation.name] = outputCreation;
        }

        for (auto& [name, type, resourceInfo]: frameGraphOutputCreations | std::views::values)
            CreateFrameGraphResource(name, type, resourceInfo);
    }

    void VulkanRenderer::CreateFrameGraphInputs()
    {
        // Lights Buffer
        {
            FrameGraphResourceInputCreation inputCreation{};
            inputCreation.name = "lights_buffer";
            inputCreation.type = FrameGraphResourceType::Buffer;
            inputCreation.resourceInfo.buffer.size = sizeof(LightsBuffer);
            inputCreation.resourceInfo.buffer.usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
            inputCreation.resourceInfo.buffer.memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;

            frameGraphInputCreations[inputCreation.name] = inputCreation;
        }

        // Camera Buffer
        {
            FrameGraphResourceInputCreation inputCreation{};
            inputCreation.name = "camera_buffer";
            inputCreation.type = FrameGraphResourceType::Buffer;
            inputCreation.resourceInfo.buffer.size = sizeof(CameraBuffer);
            inputCreation.resourceInfo.buffer.usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
            inputCreation.resourceInfo.buffer.memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;

            frameGraphInputCreations[inputCreation.name] = inputCreation;
        }

        // BRDF LUT
        {
            FrameGraphResourceInputCreation inputCreation{};
            inputCreation.name = "brdflut_texture";
            inputCreation.type = FrameGraphResourceType::Texture;
            inputCreation.resourceInfo.texture.textureCreateInfo = {};
            inputCreation.resourceInfo.texture.textureCreateInfo.width = 512;
            inputCreation.resourceInfo.texture.textureCreateInfo.height = 512;
            inputCreation.resourceInfo.texture.textureCreateInfo.format = ImageFormat::RGBA16_SFLOAT;
            inputCreation.resourceInfo.texture.textureCreateInfo.imageLayout = ImageUtils::GetLayoutFromFormat(ImageFormat::RGBA16_SFLOAT);

            frameGraphInputCreations[inputCreation.name] = inputCreation;
        }

        // Prefilter Map
        {
            FrameGraphResourceInputCreation inputCreation{};
            inputCreation.name = "prefilter_map_texture";
            inputCreation.type = FrameGraphResourceType::TextureCube;
            inputCreation.resourceInfo.texture.textureCreateInfo.width = 256;
            inputCreation.resourceInfo.texture.textureCreateInfo.height = 256;
            inputCreation.resourceInfo.texture.textureCreateInfo.format = ImageFormat::RGBA16_SFLOAT;
            inputCreation.resourceInfo.texture.textureCreateInfo.imageLayout = ImageUtils::GetLayoutFromFormat(ImageFormat::RGBA16_SFLOAT);
            inputCreation.resourceInfo.texture.textureCreateInfo.addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            inputCreation.resourceInfo.texture.textureCreateInfo.mipLevels = 6;
            inputCreation.resourceInfo.texture.textureCreateInfo.arrayLayers = 6;
            inputCreation.resourceInfo.texture.textureCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
            inputCreation.resourceInfo.texture.textureCreateInfo.isCubeMap = true;

            frameGraphInputCreations[inputCreation.name] = inputCreation;
        }

        // Irradiance Map
        {
            FrameGraphResourceInputCreation inputCreation{};
            inputCreation.name = "irradiance_map_texture";
            inputCreation.type = FrameGraphResourceType::TextureCube;
            inputCreation.resourceInfo.texture.textureCreateInfo = {};
            inputCreation.resourceInfo.texture.textureCreateInfo.width = 32;
            inputCreation.resourceInfo.texture.textureCreateInfo.height = 32;
            inputCreation.resourceInfo.texture.textureCreateInfo.format = ImageFormat::RGBA16_SFLOAT;
            inputCreation.resourceInfo.texture.textureCreateInfo.imageLayout = ImageUtils::GetLayoutFromFormat(ImageFormat::RGBA16_SFLOAT);
            inputCreation.resourceInfo.texture.textureCreateInfo.addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            inputCreation.resourceInfo.texture.textureCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
            inputCreation.resourceInfo.texture.textureCreateInfo.isCubeMap = true;
            inputCreation.resourceInfo.texture.textureCreateInfo.arrayLayers = 6;

            frameGraphInputCreations[inputCreation.name] = inputCreation;
        }

        // Directional Shadow Map
        {
            const uint16_t SHADOW_MAP_RESOLUTION = EnumValue(scene.directionalLight.shadowMapResolution);

            FrameGraphResourceInputCreation inputCreation{};
            inputCreation.name = "directional_shadow_map";
            inputCreation.type = FrameGraphResourceType::Texture;
            inputCreation.resourceInfo.texture.textureCreateInfo = {};
            inputCreation.resourceInfo.texture.textureCreateInfo.width = SHADOW_MAP_RESOLUTION;
            inputCreation.resourceInfo.texture.textureCreateInfo.height = SHADOW_MAP_RESOLUTION;
            inputCreation.resourceInfo.texture.textureCreateInfo.format = ImageFormat::DEPTH32;
            inputCreation.resourceInfo.texture.textureCreateInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
            inputCreation.resourceInfo.texture.textureCreateInfo.addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            inputCreation.resourceInfo.texture.textureCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
            inputCreation.resourceInfo.texture.textureCreateInfo.arrayLayers = SHADOW_MAP_CASCADE_COUNT;
            inputCreation.resourceInfo.texture.textureCreateInfo.compareEnabled = true;
            inputCreation.resourceInfo.texture.textureCreateInfo.compareOp = VK_COMPARE_OP_LESS;

            frameGraphInputCreations[inputCreation.name] = inputCreation;
        }

        for (auto& [name, type, resourceInfo]: frameGraphInputCreations | std::views::values)
        {
            CreateFrameGraphResource(name, type, resourceInfo);
        }
    }

    void VulkanRenderer::CreateFrameGraphResource(const char* resourceName, FrameGraphResourceType type,
                                                  FrameGraphResourceCreateInfo& createInfo)
    {
        if (frameGraphResourceHandles.contains(resourceName))
        {
            FrameGraphResource* resource = frameGraphResources.Get(frameGraphResourceHandles[resourceName].index);

            if (type == FrameGraphResourceType::Texture || type == FrameGraphResourceType::TextureCube)
                device->DestroyTexture(resource->resourceInfo.texture.textureHandle);

            if (type == FrameGraphResourceType::Buffer)
                device->DestroyBuffer(resource->resourceInfo.buffer.allocatedBuffer);

            frameGraphResourceHandles.erase(resourceName);
            frameGraphResources.Release(resource);
        }

        if (type == FrameGraphResourceType::Texture || type == FrameGraphResourceType::TextureCube)
        {
            createInfo.texture.textureHandle = device->CreateTexture(createInfo.texture.textureCreateInfo);
        }

        if (type == FrameGraphResourceType::Buffer)
        {
            createInfo.buffer.allocatedBuffer = device->CreateBuffer(
                createInfo.buffer.size,
                createInfo.buffer.usageFlags,
                createInfo.buffer.memoryUsage
            );
        }

        FrameGraphResource* graphResource = frameGraphResources.Obtain();
        graphResource->name = resourceName;
        graphResource->type = type;
        graphResource->resourceInfo = createInfo;

        frameGraphResourceHandles[graphResource->name] = {graphResource->index};
        renderPassResourceMap[graphResource->name] = *graphResource;
    }
}
