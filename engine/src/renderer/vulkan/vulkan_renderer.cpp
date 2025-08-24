#include "renderer/vulkan/vulkan_renderer.h"

#include <ranges>
#include <renderer/graph/frame_graph_renderpass.h>
#include <renderer/vulkan/vulkan_texture.h>
#include <renderer/vulkan/vulkan_utils.h>
#include <renderer/vulkan/pass/gbufferPass.h>
#include <renderer/vulkan/pass/infinite_grid_pass.h>
#include <renderer/vulkan/pass/lighting_pass.h>
#include <renderer/vulkan/pass/shadow_map_pass.h>
#include <renderer/vulkan/pass/skybox_pass.h>
#include <renderer/vulkan/pass/ui_pass.h>
#include <renderer/vulkan/pass/lighting/brdf_lut_pass.h>
#include <renderer/vulkan/pass/lighting/irradiance_map_pass.h>
#include <renderer/vulkan/pass/lighting/prefilter_map_pass.h>
#include <renderer/vulkan/pass/post_processing/ssao_pass.h>

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

        frameGraph = CreateScope<FrameGraph>(device);
        frameGraph->SetResolution(renderResolution);

        frameGraphResources.Init(128);

        CreateSwapchain();
    }

    void VulkanRenderer::InitializeRenderPasses()
    {
        // Skybox pass
        {
            frameGraphRenderPasses["SkyboxPass"]->Reset();

            frameGraphRenderPasses["SkyboxPass"]->AddInput(renderPassResourceMap["camera_buffer"]);

            frameGraphRenderPasses["SkyboxPass"]->AddOutput({
                .resource = renderPassResourceMap["hdr_image"],
                .loadOp = RenderPassOperation::LoadOp::Clear,
                .storeOp = RenderPassOperation::StoreOp::Store
            });
            frameGraphRenderPasses["SkyboxPass"]->AddOutput({
                .resource = renderPassResourceMap["depth_map"],
                .loadOp = RenderPassOperation::LoadOp::Clear,
                .storeOp = RenderPassOperation::StoreOp::Store
            });

            frameGraphRenderPasses["SkyboxPass"]->Init();
        }

        // Grid pass
        {
            frameGraphRenderPasses["InfiniteGridPass"]->Reset();

            frameGraphRenderPasses["InfiniteGridPass"]->AddInput(renderPassResourceMap["camera_buffer"]);

            frameGraphRenderPasses["InfiniteGridPass"]->AddOutput({
                .resource = renderPassResourceMap["hdr_image"],
                .loadOp = RenderPassOperation::LoadOp::Load,
                .storeOp = RenderPassOperation::StoreOp::Store
            });

            frameGraphRenderPasses["InfiniteGridPass"]->AddOutput({
                .resource = renderPassResourceMap["depth_map"],
                .loadOp = RenderPassOperation::LoadOp::Load,
                .storeOp = RenderPassOperation::StoreOp::Store
            });

            frameGraphRenderPasses["InfiniteGridPass"]->Init();
        }

        // GBuffer pass
        {
            frameGraphRenderPasses["GBufferPass"]->Reset();

            frameGraphRenderPasses["GBufferPass"]->AddInput(renderPassResourceMap["camera_buffer"]);

            frameGraphRenderPasses["GBufferPass"]->AddOutput({
                .resource = renderPassResourceMap["viewspace_normal"],
                .loadOp = RenderPassOperation::LoadOp::Clear,
                .storeOp = RenderPassOperation::StoreOp::Store
            });

            frameGraphRenderPasses["GBufferPass"]->AddOutput({
                .resource = renderPassResourceMap["viewspace_position"],
                .loadOp = RenderPassOperation::LoadOp::Clear,
                .storeOp = RenderPassOperation::StoreOp::Store
            });

            frameGraphRenderPasses["GBufferPass"]->AddOutput({
                .resource = renderPassResourceMap["depth_map"],
                .loadOp = RenderPassOperation::LoadOp::Clear,
                .storeOp = RenderPassOperation::StoreOp::Store
            });

            frameGraphRenderPasses["GBufferPass"]->Init();
        }

        // Lighting pass
        {
            frameGraphRenderPasses["LightingPass"]->Reset();

            frameGraphRenderPasses["LightingPass"]->AddInput(renderPassResourceMap["camera_buffer"]);
            frameGraphRenderPasses["LightingPass"]->AddInput(renderPassResourceMap["lights_buffer"]);
            frameGraphRenderPasses["LightingPass"]->AddInput(renderPassResourceMap["directional_shadow_map"]);
            frameGraphRenderPasses["LightingPass"]->AddInput(renderPassResourceMap["irradiance_map_texture"]);
            frameGraphRenderPasses["LightingPass"]->AddInput(renderPassResourceMap["ssao_texture"]);
            frameGraphRenderPasses["LightingPass"]->AddInput(renderPassResourceMap["prefilter_map_texture"]);
            frameGraphRenderPasses["LightingPass"]->AddInput(renderPassResourceMap["brdflut_texture"]);

            frameGraphRenderPasses["LightingPass"]->AddOutput({
                .resource = renderPassResourceMap["hdr_image"],
                .loadOp = RenderPassOperation::LoadOp::Load,
                .storeOp = RenderPassOperation::StoreOp::Store
            });

            frameGraphRenderPasses["LightingPass"]->AddOutput({
                .resource = renderPassResourceMap["depth_map"],
                .loadOp = RenderPassOperation::LoadOp::Load,
                .storeOp = RenderPassOperation::StoreOp::Store
            });

            frameGraphRenderPasses["LightingPass"]->Init();
        }

        // SSAO pass
        {
            frameGraphRenderPasses["SSAOPass"]->Reset();

            frameGraphRenderPasses["SSAOPass"]->AddInput(renderPassResourceMap["viewspace_normal"]);
            frameGraphRenderPasses["SSAOPass"]->AddInput(renderPassResourceMap["viewspace_position"]);
            frameGraphRenderPasses["SSAOPass"]->AddInput(renderPassResourceMap["depth_map"]);
            frameGraphRenderPasses["SSAOPass"]->AddInput(renderPassResourceMap["camera_buffer"]);

            frameGraphRenderPasses["SSAOPass"]->AddOutput({
                .resource = renderPassResourceMap["ssao_texture"],
                .loadOp = RenderPassOperation::LoadOp::Clear,
                .storeOp = RenderPassOperation::StoreOp::Store
            });

            frameGraphRenderPasses["SSAOPass"]->Init();
        }

        // Shadow map pass
        {
            frameGraphRenderPasses["ShadowMapPass"]->Reset();

            frameGraphRenderPasses["ShadowMapPass"]->AddOutput({
                .resource = renderPassResourceMap["directional_shadow_map"],
                .loadOp = RenderPassOperation::LoadOp::Clear,
                .storeOp = RenderPassOperation::StoreOp::Store
            });

            frameGraphRenderPasses["ShadowMapPass"]->Init();
        }

        // Tone Mapping pass
        {
            frameGraphRenderPasses["ToneMappingPass"]->Reset();

            frameGraphRenderPasses["ToneMappingPass"]->AddInput(renderPassResourceMap["hdr_image"]);

            frameGraphRenderPasses["ToneMappingPass"]->AddOutput({
                .resource = renderPassResourceMap["main_frame_color"],
                .loadOp = RenderPassOperation::LoadOp::Clear,
                .storeOp = RenderPassOperation::StoreOp::Store
            });
            frameGraphRenderPasses["ToneMappingPass"]->Init();
        }

        // UI pass
        {
            frameGraphRenderPasses["UiPass"]->Reset();
            frameGraphRenderPasses["UiPass"]->AddOutput({
                .resource = renderPassResourceMap["main_frame_color"],
                .loadOp = RenderPassOperation::LoadOp::Load,
                .storeOp = RenderPassOperation::StoreOp::Store
            });
            frameGraphRenderPasses["UiPass"]->Init();
        }
    }

    void VulkanRenderer::InitializeIBLPasses()
    {
        // BRDF LUT pass
        {
            frameGraphRenderPasses["BrdfLUTPass"]->Reset();

            frameGraphRenderPasses["BrdfLUTPass"]->AddOutput({
                .resource = renderPassResourceMap["brdflut_texture"],
                .loadOp = RenderPassOperation::LoadOp::Clear,
                .storeOp = RenderPassOperation::StoreOp::Store
            });

            frameGraphRenderPasses["BrdfLUTPass"]->Init();
        }

        // Prefilter map pass
        {
            frameGraphRenderPasses["PrefilterMapPass"]->Reset();

            frameGraphRenderPasses["PrefilterMapPass"]->AddOutput({
                .resource = renderPassResourceMap["prefilter_map_texture"],
                .loadOp = RenderPassOperation::LoadOp::Clear,
                .storeOp = RenderPassOperation::StoreOp::Store
            });

            static_cast<PrefilterMapPass*>(frameGraphRenderPasses["PrefilterMapPass"])->SetCubemapTexture(sceneGraph->skyboxTexture);

            frameGraphRenderPasses["PrefilterMapPass"]->Init();
        }

        // Irradiance map pass
        {
            frameGraphRenderPasses["IrradianceMapPass"]->Reset();

            frameGraphRenderPasses["IrradianceMapPass"]->AddOutput({
                .resource = renderPassResourceMap["irradiance_map_texture"],
                .loadOp = RenderPassOperation::LoadOp::Clear,
                .storeOp = RenderPassOperation::StoreOp::Store
            });

            static_cast<IrradianceMapPass*>(frameGraphRenderPasses["IrradianceMapPass"])->SetCubemapTexture(sceneGraph->skyboxTexture);

            frameGraphRenderPasses["IrradianceMapPass"]->Init();
        }
    }

    void VulkanRenderer::LoadScene(const std::string& gltfPath, const std::string& hdrPath)
    {
        isSceneLoaded = false;

        LOG_TRACE("Load scene");
        sceneGraph = ResourceManager::LoadSceneGraph(device, gltfPath, hdrPath);
        sceneGraph->directionalLight.direction = normalize(glm::vec3(0.0f, -2.0f, -1.0f));

        CreateFrameGraphInputs();
        CreateFrameGraphOutputs();

        AddRenderPass<IrradianceMapPass>("IrradianceMapPass");
        AddRenderPass<BrdfLUTPass>("BrdfLUTPass");
        AddRenderPass<PrefilterMapPass>("PrefilterMapPass");

        AddRenderPass<GBufferPass>("GBufferPass");
        AddRenderPass<LightingPass>("LightingPass");
        AddRenderPass<ShadowMapPass>("ShadowMapPass");
        AddRenderPass<SSAOPass>("SSAOPass");
        AddRenderPass<ToneMappingPass>("ToneMappingPass");
        AddRenderPass<SkyboxPass>("SkyboxPass");
        AddRenderPass<InfiniteGridPass>("InfiniteGridPass");
        AddRenderPass<UiPass>("UiPass");

        frameGraph->AddRenderPass<GBufferPass>("GBufferPass");
        frameGraph->AddRenderPass<ShadowMapPass>("ShadowMapPass");
        frameGraph->AddRenderPass<SkyboxPass>("SkyboxPass");
        frameGraph->AddRenderPass<SSAOPass>("SSAOPass");
        frameGraph->AddRenderPass<LightingPass>("LightingPass");
        frameGraph->AddRenderPass<InfiniteGridPass>("InfiniteGridPass");
        frameGraph->AddRenderPass<ToneMappingPass>("ToneMappingPass");
        frameGraph->AddRenderPass<UiPass>("UiPass");

        InitializeRenderPasses();
        InitializeIBLPasses();

        frameGraph->Compile();


        // IBL and reflection calculations
        device->ImmediateSubmit([&](VkCommandBuffer commandBuffer) {
            frameGraphRenderPasses["IrradianceMapPass"]->Render(commandBuffer, sceneGraph);
            frameGraphRenderPasses["BrdfLUTPass"]->Render(commandBuffer, sceneGraph);
            frameGraphRenderPasses["PrefilterMapPass"]->Render(commandBuffer, sceneGraph);

            delete frameGraphRenderPasses.find("IrradianceMapPass")->second;
            frameGraphRenderPasses.erase("IrradianceMapPass");

            delete frameGraphRenderPasses.find("BrdfLUTPass")->second;
            frameGraphRenderPasses.erase("BrdfLUTPass");

            delete frameGraphRenderPasses.find("PrefilterMapPass")->second;
            frameGraphRenderPasses.erase("PrefilterMapPass");
        });

        isSceneLoaded = true;
    }

    void VulkanRenderer::Draw(const float deltaTime, Camera& camera)
    {
        if (!isSceneLoaded) return;

        device->DrawFrame(vulkanSwapChain->GetSwapChain(),
                          [&](const VkCommandBuffer cmd, const uint32_t imgIndex) {
                              //RotateLight(deltaTime);
                              sceneGraph->directionalLight.UpdateCascades(camera);
                              UpdateLightsBuffer();
                              UpdateCameraBuffer(camera);

                              DrawFrame(cmd, imgIndex);
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

    void VulkanRenderer::ResizeSwapchain()
    {
        IdleWait();

        CreateSwapchain();
        CreateFrameGraphOutputs();

        for (const auto& renderPass: frameGraphRenderPasses | std::views::values)
            renderPass->Resize(renderResolution);

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
        sceneGraph->directionalLight.direction = normalize(glm::vec3(glm::vec4(sceneGraph->directionalLight.direction, 1.0f) * rot_mat));
    }

    void VulkanRenderer::UpdateLightsBuffer()
    {
        LightsBuffer bufferData;

        for (size_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
        {
            bufferData.lightProjection[i] = sceneGraph->directionalLight.cascades[i].viewProjMatrix;
            bufferData.cascadeSplits[i].x = sceneGraph->directionalLight.cascades[i].splitDepth;
        }

        bufferData.direction = glm::vec4(normalize(sceneGraph->directionalLight.direction), 0.0);
        bufferData.color = glm::vec4(sceneGraph->directionalLight.color, 1.0f);
        bufferData.intensity = sceneGraph->directionalLight.intensity;
        bufferData.ambientIntensity = sceneGraph->directionalLight.ambientIntensity;
        bufferData.bias = sceneGraph->directionalLight.bias;

        memcpy(renderPassResourceMap["lights_buffer"].resourceInfo.buffer.allocatedBuffer.GetData(), &bufferData, sizeof(LightsBuffer));
    }

    void VulkanRenderer::PresentFrame(const VkCommandBuffer& commandBuffer, uint32_t imageIndex)
    {
        VkImage swapchainImage = vulkanSwapChain->GetImages()[imageIndex];
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

    void VulkanRenderer::DrawFrame(const VkCommandBuffer& commandBuffer, uint32_t imageIndex)
    {
        if (vulkanSwapChain->GetExtent().width != renderResolution.width || vulkanSwapChain->GetExtent().height != renderResolution.height)
            return;

        frameGraphRenderPasses["GBufferPass"]->Render(commandBuffer, sceneGraph);
        frameGraphRenderPasses["ShadowMapPass"]->Render(commandBuffer, sceneGraph);
        frameGraphRenderPasses["SSAOPass"]->Render(commandBuffer, sceneGraph);
        frameGraphRenderPasses["SkyboxPass"]->Render(commandBuffer, sceneGraph);

        VulkanTexture* depthMap = device->GetTexture(renderPassResourceMap["depth_map"].resourceInfo.texture.textureHandle);
        VulkanUtils::TransitionImageLayout(commandBuffer, depthMap->allocatedImage,
                                           VK_IMAGE_ASPECT_DEPTH_BIT,
                                           VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                           VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);

        frameGraphRenderPasses["LightingPass"]->Render(commandBuffer, sceneGraph);
        frameGraphRenderPasses["InfiniteGridPass"]->Render(commandBuffer, sceneGraph);

        VulkanUtils::TransitionImageLayout(commandBuffer, depthMap->allocatedImage,
                                           VK_IMAGE_ASPECT_DEPTH_BIT,
                                           VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
                                           VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

        frameGraphRenderPasses["ToneMappingPass"]->Render(commandBuffer, sceneGraph);
        frameGraphRenderPasses["UiPass"]->Render(commandBuffer, sceneGraph);

        PresentFrame(commandBuffer, imageIndex);
    }

    void VulkanRenderer::CreateFrameGraphOutputs()
    {
        // Viewspace Normal
        {
            FrameGraphResourceCreate outputCreation{};
            outputCreation.name = "viewspace_normal";
            outputCreation.type = FrameGraphResourceType::Texture;
            outputCreation.resourceInfo.texture.textureCreateInfo = {
                .width = renderResolution.width,
                .height = renderResolution.height,
                .format = ImageFormat::RGBA32_SFLOAT,
                .addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            };

            frameGraphOutputCreations[outputCreation.name] = outputCreation;
        }

        // Viewspace Position
        {
            FrameGraphResourceCreate outputCreation{};
            outputCreation.name = "viewspace_position";
            outputCreation.type = FrameGraphResourceType::Texture;
            outputCreation.resourceInfo.texture.textureCreateInfo = {
                .width = renderResolution.width,
                .height = renderResolution.height,
                .format = ImageFormat::RGBA32_SFLOAT,
                .addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            };

            frameGraphOutputCreations[outputCreation.name] = outputCreation;
        }

        // Depth Map
        {
            FrameGraphResourceCreate outputCreation{};
            outputCreation.name = "depth_map";
            outputCreation.type = FrameGraphResourceType::Texture;
            outputCreation.resourceInfo.texture.textureCreateInfo = {
                .width = renderResolution.width,
                .height = renderResolution.height,
                .format = ImageFormat::DEPTH24_STENCIL8,
                .addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            };

            frameGraphOutputCreations[outputCreation.name] = outputCreation;
        }

        // SSAO Texture
        {
            FrameGraphResourceCreate outputCreation{};
            outputCreation.name = "ssao_texture";
            outputCreation.type = FrameGraphResourceType::Texture;
            outputCreation.resourceInfo.texture.textureCreateInfo = {
                .width = static_cast<uint32_t>(renderResolution.width * 0.5),
                .height = static_cast<uint32_t>(renderResolution.height * 0.5),
                .format = ImageFormat::R8_UNORM,
            };

            frameGraphOutputCreations[outputCreation.name] = outputCreation;
        }

        // HDR Image
        {
            FrameGraphResourceCreate outputCreation{};
            outputCreation.name = "hdr_image";
            outputCreation.type = FrameGraphResourceType::Texture;
            outputCreation.resourceInfo.texture.textureCreateInfo = {
                .width = renderResolution.width,
                .height = renderResolution.height,
                .format = ImageFormat::RGBA16_SFLOAT,
                .addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            };

            frameGraphOutputCreations[outputCreation.name] = outputCreation;
        }

        // Main Frame Color
        {
            FrameGraphResourceCreate outputCreation{};
            outputCreation.name = "main_frame_color";
            outputCreation.type = FrameGraphResourceType::Texture;
            outputCreation.resourceInfo.texture.textureCreateInfo = {
                .width = renderResolution.width,
                .height = renderResolution.height,
                .format = ImageFormat::RGBA8_UNORM,
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
            FrameGraphResourceCreate inputCreation{};
            inputCreation.name = "lights_buffer";
            inputCreation.type = FrameGraphResourceType::Buffer;
            inputCreation.resourceInfo.buffer.size = sizeof(LightsBuffer);
            inputCreation.resourceInfo.buffer.usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
            inputCreation.resourceInfo.buffer.memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;

            frameGraphInputCreations[inputCreation.name] = inputCreation;

            frameGraph->CreateResource(inputCreation);
        }

        // Camera Buffer
        {
            FrameGraphResourceCreate inputCreation{};
            inputCreation.name = "camera_buffer";
            inputCreation.type = FrameGraphResourceType::Buffer;
            inputCreation.resourceInfo.buffer.size = sizeof(CameraBuffer);
            inputCreation.resourceInfo.buffer.usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
            inputCreation.resourceInfo.buffer.memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;

            frameGraphInputCreations[inputCreation.name] = inputCreation;

            frameGraph->CreateResource(inputCreation);
        }

        // BRDF LUT
        {
            FrameGraphResourceCreate inputCreation{};
            inputCreation.name = "brdflut_texture";
            inputCreation.type = FrameGraphResourceType::Texture;
            inputCreation.resourceInfo.texture.textureCreateInfo = {};
            inputCreation.resourceInfo.texture.textureCreateInfo.width = 512;
            inputCreation.resourceInfo.texture.textureCreateInfo.height = 512;
            inputCreation.resourceInfo.texture.textureCreateInfo.format = ImageFormat::RGBA16_SFLOAT;

            frameGraphInputCreations[inputCreation.name] = inputCreation;

            frameGraph->CreateResource(inputCreation);
        }

        // Prefilter Map
        {
            FrameGraphResourceCreate inputCreation{};
            inputCreation.name = "prefilter_map_texture";
            inputCreation.type = FrameGraphResourceType::TextureCube;
            inputCreation.resourceInfo.texture.textureCreateInfo.width = 256;
            inputCreation.resourceInfo.texture.textureCreateInfo.height = 256;
            inputCreation.resourceInfo.texture.textureCreateInfo.format = ImageFormat::RGBA16_SFLOAT;
            inputCreation.resourceInfo.texture.textureCreateInfo.addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            inputCreation.resourceInfo.texture.textureCreateInfo.mipLevels = 6;
            inputCreation.resourceInfo.texture.textureCreateInfo.arrayLayers = 6;
            inputCreation.resourceInfo.texture.textureCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
            inputCreation.resourceInfo.texture.textureCreateInfo.isCubeMap = true;

            frameGraphInputCreations[inputCreation.name] = inputCreation;

            frameGraph->CreateResource(inputCreation);
        }

        // Irradiance Map
        {
            FrameGraphResourceCreate inputCreation{};
            inputCreation.name = "irradiance_map_texture";
            inputCreation.type = FrameGraphResourceType::TextureCube;
            inputCreation.resourceInfo.texture.textureCreateInfo = {};
            inputCreation.resourceInfo.texture.textureCreateInfo.width = 32;
            inputCreation.resourceInfo.texture.textureCreateInfo.height = 32;
            inputCreation.resourceInfo.texture.textureCreateInfo.format = ImageFormat::RGBA16_SFLOAT;
            inputCreation.resourceInfo.texture.textureCreateInfo.addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            inputCreation.resourceInfo.texture.textureCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
            inputCreation.resourceInfo.texture.textureCreateInfo.isCubeMap = true;
            inputCreation.resourceInfo.texture.textureCreateInfo.arrayLayers = 6;

            frameGraphInputCreations[inputCreation.name] = inputCreation;

            frameGraph->CreateResource(inputCreation);
        }

        // Directional Shadow Map
        {
            const uint16_t SHADOW_MAP_RESOLUTION = EnumValue(sceneGraph->directionalLight.shadowMapResolution);

            FrameGraphResourceCreate inputCreation{};
            inputCreation.name = "directional_shadow_map";
            inputCreation.type = FrameGraphResourceType::Texture;
            inputCreation.resourceInfo.texture.textureCreateInfo = {};
            inputCreation.resourceInfo.texture.textureCreateInfo.width = SHADOW_MAP_RESOLUTION;
            inputCreation.resourceInfo.texture.textureCreateInfo.height = SHADOW_MAP_RESOLUTION;
            inputCreation.resourceInfo.texture.textureCreateInfo.format = ImageFormat::DEPTH32;
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

    void VulkanRenderer::DestroyResource(const char* resourceName)
    {
        FrameGraphResource* resource = frameGraphResources.Get(frameGraphResourceHandles[resourceName].index);

        if (resource->type == FrameGraphResourceType::Texture || resource->type == FrameGraphResourceType::TextureCube)
            device->DestroyTexture(resource->resourceInfo.texture.textureHandle);

        if (resource->type == FrameGraphResourceType::Buffer)
            device->DestroyBuffer(resource->resourceInfo.buffer.allocatedBuffer);

        frameGraphResourceHandles.erase(resourceName);
        frameGraphResources.Release(resource);
    }

    void VulkanRenderer::CreateFrameGraphResource(const char* resourceName, FrameGraphResourceType type,
                                                  FrameGraphResourceCreateInfo& createInfo)
    {
        if (frameGraphResourceHandles.contains(resourceName))
            DestroyResource(resourceName);

        switch (type)
        {
            case FrameGraphResourceType::Texture:
                createInfo.texture.textureHandle = device->CreateTexture(createInfo.texture.textureCreateInfo);
                break;
            case FrameGraphResourceType::TextureCube:
                createInfo.texture.textureHandle = device->CreateTexture(createInfo.texture.textureCreateInfo);
                break;
            case FrameGraphResourceType::Buffer:
                createInfo.buffer.allocatedBuffer = device->CreateBuffer(createInfo.buffer.size,
                                                                         createInfo.buffer.usageFlags,
                                                                         createInfo.buffer.memoryUsage);
                break;
            default:
                ASSERT(false, "Unknown frame graph resource type");
        }

        FrameGraphResource* graphResource = frameGraphResources.Obtain();
        graphResource->name = resourceName;
        graphResource->type = type;
        graphResource->resourceInfo = createInfo;

        frameGraphResourceHandles[graphResource->name] = {graphResource->index};
        renderPassResourceMap[graphResource->name] = *graphResource;
    }
}
