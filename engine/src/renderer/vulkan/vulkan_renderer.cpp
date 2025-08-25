#include "renderer/vulkan/vulkan_renderer.h"

#include <ranges>
#include <renderer/vulkan/vulkan_texture.h>
#include <renderer/vulkan/vulkan_utils.h>
#include <renderer/vulkan/pass/gbufferPass.h>
#include <renderer/vulkan/pass/infinite_grid_pass.h>
#include <renderer/vulkan/pass/lighting_pass.h>
#include <renderer/vulkan/pass/shadow_map_pass.h>
#include <renderer/vulkan/pass/skybox_pass.h>
#include <renderer/vulkan/pass/ui_pass.h>
#include <renderer/vulkan/pass/lighting/brdf_lut_pass.h>
#include <renderer/vulkan/pass/post_processing/ssao_pass.h>
#include <renderer/vulkan/pass/post_processing/tone_mapping_pass.h>
#include <util/thread_pool.h>
#include <util/timer.h>

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
        device->DestroyBuffer(cameraBuffer->allocatedBuffer);
        device->DestroyBuffer(lightBuffer->allocatedBuffer);

        device->DestroyTexture(brdfLUT->textureHandle);
        device->DestroyTexture(prefilteredMap->textureHandle);
        device->DestroyTexture(irradianceMap->textureHandle);

        delete brdfLUT;
        delete prefilteredMap;
        delete irradianceMap;

        delete cameraBuffer;
        delete lightBuffer;
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
        frameGraph = CreateScope<FrameGraph::FrameGraph>(device);

        CreateSwapchain();
    }

    void VulkanRenderer::CalculateIBL()
    {
        irradiancePass = new IrradianceMapPass(device);
        brdfLutPass = new BrdfLUTPass(device);
        prefilterPass = new PrefilterMapPass(device);

        // Init passes
        {
            // BRDF LUT pass
            {
                brdfLutPass->Reset();
                brdfLutPass->AddOutput({
                    .resource = brdfLUT,
                    .loadOp = RenderPassOperation::LoadOp::Clear,
                    .storeOp = RenderPassOperation::StoreOp::Store
                });
                brdfLutPass->Init();
            }

            // Prefilter map pass
            {
                prefilterPass->Reset();
                prefilterPass->AddOutput({
                    .resource = prefilteredMap,
                    .loadOp = RenderPassOperation::LoadOp::Clear,
                    .storeOp = RenderPassOperation::StoreOp::Store
                });
                prefilterPass->SetCubemapTexture(sceneGraph->skyboxTexture);
                prefilterPass->Init();
            }

            // Irradiance map pass
            {
                irradiancePass->Reset();
                irradiancePass->AddOutput({
                    .resource = irradianceMap,
                    .loadOp = RenderPassOperation::LoadOp::Clear,
                    .storeOp = RenderPassOperation::StoreOp::Store
                });
                irradiancePass->SetCubemapTexture(sceneGraph->skyboxTexture);
                irradiancePass->Init();
            }
        }

        // IBL and reflection calculations
        device->ImmediateSubmit([&](const VkCommandBuffer commandBuffer) {
            irradiancePass->Render(commandBuffer, sceneGraph);
            brdfLutPass->Render(commandBuffer, sceneGraph);
            prefilterPass->Render(commandBuffer, sceneGraph);

            delete irradiancePass;
            delete brdfLutPass;
            delete prefilterPass;
        });
    }

    void VulkanRenderer::LoadScene(const std::string& gltfPath, const std::string& hdrPath)
    {
        isSceneLoaded = false;

        LOG_TRACE("Load scene");
        sceneGraph = ResourceManager::LoadSceneGraph(device, gltfPath, hdrPath);
        sceneGraph->directionalLight.direction = normalize(glm::vec3(0.0f, -2.0f, -1.0f));

        CreateExternalResources();
        CalculateIBL();

        frameGraph->Compile(renderResolution);

        isSceneLoaded = true;
    }

    void VulkanRenderer::Draw(const float deltaTime, Camera& camera)
    {
        if (!isSceneLoaded) return;

        device->DrawFrame(vulkanSwapChain->GetSwapChain(),
                          [&](const VkCommandBuffer cmd, const uint32_t imgIndex) {
                              RotateLight(deltaTime);
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
        frameGraph->Resize(renderResolution);
    }

    void VulkanRenderer::UpdateCameraBuffer(Camera& camera)
    {
        const CameraBuffer bufferData{
            .proj = camera.GetProjection(),
            .view = camera.GetView(),
            .cameraPosition = camera.GetTransform().m_Position
        };

        memcpy(cameraBuffer->allocatedBuffer.GetData(), &bufferData, sizeof(CameraBuffer));
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

        memcpy(lightBuffer->allocatedBuffer.GetData(), &bufferData, sizeof(LightsBuffer));
    }

    void VulkanRenderer::PresentFrame(const VkCommandBuffer& commandBuffer, uint32_t imageIndex, TextureHandle textureToPresent)
    {
        VkImage swapchainImage = vulkanSwapChain->GetImages()[imageIndex];
        VulkanTexture* presentTexture = device->GetTexture(textureToPresent);

        VulkanUtils::TransitionImageLayout(commandBuffer, presentTexture->allocatedImage,
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

        CopyImage(commandBuffer, presentTexture->allocatedImage, swapchainImage, copyParams);

        VulkanUtils::TransitionImageLayout(commandBuffer, swapchainImage,
                                           VK_IMAGE_ASPECT_COLOR_BIT,
                                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                           VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        VulkanUtils::TransitionImageLayout(commandBuffer, presentTexture->allocatedImage,
                                           VK_IMAGE_ASPECT_COLOR_BIT,
                                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                           VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    }

    void VulkanRenderer::DrawFrame(const VkCommandBuffer& commandBuffer, uint32_t imageIndex)
    {
        if (vulkanSwapChain->GetExtent().width != renderResolution.width ||
            vulkanSwapChain->GetExtent().height != renderResolution.height)
            return;

        frameGraph->Execute(commandBuffer, sceneGraph);

        PresentFrame(commandBuffer, imageIndex, frameGraph->GetResource("main_frame_color")->textureHandle);
    }

    void VulkanRenderer::CreateExternalResources()
    {
        std::vector<FrameGraph::FrameGraphResourceCreate> frameGraphInputCreations;

        // Lights Buffer
        {
            FrameGraph::FrameGraphBufferCreateInfo bufferCreateInfo{};
            bufferCreateInfo.size = sizeof(LightsBuffer);
            bufferCreateInfo.usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
            bufferCreateInfo.memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;

            FrameGraph::FrameGraphResourceCreate inputCreation{};
            inputCreation.name = "lights_buffer";
            inputCreation.type = FrameGraph::FrameGraphResourceType::Buffer;
            inputCreation.bufferCreateInfo = bufferCreateInfo;

            lightBuffer = CreateFrameGraphBufferResource(inputCreation.name, inputCreation.bufferCreateInfo);
            frameGraph->AddExternalResource(inputCreation.name, lightBuffer);
        }

        // Camera Buffer
        {
            FrameGraph::FrameGraphBufferCreateInfo bufferCreateInfo{};
            bufferCreateInfo.size = sizeof(CameraBuffer);
            bufferCreateInfo.usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
            bufferCreateInfo.memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;

            FrameGraph::FrameGraphResourceCreate inputCreation{};
            inputCreation.name = "camera_buffer";
            inputCreation.type = FrameGraph::FrameGraphResourceType::Buffer;
            inputCreation.bufferCreateInfo = bufferCreateInfo;

            cameraBuffer = CreateFrameGraphBufferResource(inputCreation.name, inputCreation.bufferCreateInfo);
            frameGraph->AddExternalResource(inputCreation.name, cameraBuffer);
        }

        // BRDF LUT
        {
            TextureCreateInfo textureCreateInfo{};
            textureCreateInfo.resolution = {512, 512};
            textureCreateInfo.format = ImageFormat::RGBA16_SFLOAT;

            FrameGraph::FrameGraphResourceCreate inputCreation{};
            inputCreation.name = "brdflut_texture";
            inputCreation.type = FrameGraph::FrameGraphResourceType::Texture;
            inputCreation.textureInfo = textureCreateInfo;

            brdfLUT = CreateFrameGraphTextureResource(inputCreation.name, inputCreation.textureInfo);
            frameGraph->AddExternalResource(inputCreation.name, brdfLUT);
        }

        // Prefilter Map
        {
            TextureCreateInfo textureCreateInfo{};
            textureCreateInfo.resolution = {256, 256};
            textureCreateInfo.format = ImageFormat::RGBA16_SFLOAT;
            textureCreateInfo.addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            textureCreateInfo.mipLevels = 6;
            textureCreateInfo.arrayLayers = 6;
            textureCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
            textureCreateInfo.isCubeMap = true;

            FrameGraph::FrameGraphResourceCreate inputCreation{};
            inputCreation.name = "prefilter_map_texture";
            inputCreation.type = FrameGraph::FrameGraphResourceType::Texture;
            inputCreation.textureInfo = textureCreateInfo;

            prefilteredMap = CreateFrameGraphTextureResource(inputCreation.name, inputCreation.textureInfo);
            frameGraph->AddExternalResource(inputCreation.name, prefilteredMap);
        }

        // Irradiance Map
        {
            TextureCreateInfo textureCreateInfo{};
            textureCreateInfo.resolution = {32, 32};
            textureCreateInfo.format = ImageFormat::RGBA16_SFLOAT;
            textureCreateInfo.addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            textureCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
            textureCreateInfo.isCubeMap = true;
            textureCreateInfo.arrayLayers = 6;

            FrameGraph::FrameGraphResourceCreate inputCreation{};
            inputCreation.name = "irradiance_map_texture";
            inputCreation.type = FrameGraph::FrameGraphResourceType::Texture;
            inputCreation.textureInfo = textureCreateInfo;

            irradianceMap = CreateFrameGraphTextureResource(inputCreation.name, inputCreation.textureInfo);
            frameGraph->AddExternalResource(inputCreation.name, irradianceMap);
        }
    }

    FrameGraph::FrameGraphResource* VulkanRenderer::CreateFrameGraphTextureResource(const char* resourceName, TextureCreateInfo& createInfo)
    {
        FrameGraph::FrameGraphResource* graphResource = new FrameGraph::FrameGraphResource();
        graphResource->name = resourceName;
        graphResource->type = FrameGraph::FrameGraphResourceType::Texture;
        graphResource->textureInfo = createInfo;
        graphResource->textureHandle =  device->CreateTexture(createInfo);

        return graphResource;
    }

    FrameGraph::FrameGraphResource*
    VulkanRenderer::CreateFrameGraphBufferResource(const char* resourceName, FrameGraph::FrameGraphBufferCreateInfo& createInfo)
    {
        FrameGraph::FrameGraphResource* graphResource = new FrameGraph::FrameGraphResource();
        graphResource->name = resourceName;
        graphResource->type = FrameGraph::FrameGraphResourceType::Buffer;
        graphResource->allocatedBuffer = device->CreateBuffer(
            createInfo.size,
            createInfo.usageFlags,
            createInfo.memoryUsage
        );

        return graphResource;
    }
}
