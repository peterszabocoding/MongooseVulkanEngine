#include "vulkan_renderer.h"

#include "imgui.h"
#include "vulkan_descriptor_writer.h"
#include "vulkan_framebuffer.h"
#include "resource/resource_manager.h"
#include "vulkan_pipeline.h"
#include "vulkan_device.h"
#include "vulkan_mesh.h"
#include "vulkan_shader_compiler.h"
#include "lighting/irradiance_map_generator.h"
#include "lighting/reflection_probe_generator.h"
#include "util/log.h"
#include "shaderc/shaderc.hpp"
#include "util/filesystem.h"

namespace Raytracing
{
    const glm::mat4 m_CaptureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    const glm::mat4 m_CaptureViews[6] = {
        lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
        lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))
    };

    VulkanRenderer::~VulkanRenderer() {}

    void VulkanRenderer::Init(const int width, const int height)
    {
        LOG_TRACE("VulkanRenderer::Init()");
        viewportWidth = width;
        viewportHeight = height;

        device = CreateScope<VulkanDevice>(width, height, glfwWindow);
        shaderCache = CreateScope<ShaderCache>(device.get());

        cubeMesh = ResourceManager::LoadMesh(device.get(), "resources/models/cube.obj");
        screenRect = CreateScope<VulkanMeshlet>(device.get(), Primitives::RECTANGLE_VERTICES, Primitives::RECTANGLE_INDICES);

        LOG_TRACE("Load scene");
        //scene = ResourceManager::LoadScene(device.get(), "resources/PBRCheck/pbr_check.gltf", "resources/environment/etzwihl_4k.hdr");
        scene = ResourceManager::LoadScene(device.get(), "resources/cannon/cannon.gltf", "resources/environment/etzwihl_4k.hdr");
        //scene = ResourceManager::LoadScene(device.get(), "resources/sponza/Sponza.gltf", "resources/environment/etzwihl_4k.hdr");
        //scene = ResourceManager::LoadScene(device.get(), "resources/MetalRoughSpheres/MetalRoughSpheres.gltf", "resources/environment/etzwihl_4k.hdr");
        //scene = ResourceManager::LoadScene(device.get(), "resources/gltf/multiple_spheres.gltf", "resources/environment/etzwihl_4k.hdr");
        //scene = ResourceManager::LoadScene(device.get(), "resources/chess/ABeautifulGame.gltf", "resources/environment/etzwihl_4k.hdr");
        //scene = ResourceManager::LoadScene(device.get(), "resources/DamagedHelmet/DamagedHelmet.gltf", "resources/environment/etzwihl_4k.hdr");

        scene.directionalLight.direction = normalize(glm::vec3(0.0f, -2.0f, -1.0f));

        gbufferPass = CreateScope<GBufferPass>(device.get(), scene);
        gbufferPass->SetSize(viewportWidth * resolutionScale, viewportHeight * resolutionScale);

        skyboxPass = CreateScope<SkyboxPass>(device.get(), scene);
        renderPass = CreateScope<RenderPass>(device.get(), scene);
        shadowMapPass = CreateScope<ShadowMapPass>(device.get(), scene);
        presentPass = CreateScope<PresentPass>(device.get());
        ssaoPass = CreateScope<SSAOPass>(device.get());
        gridPass = CreateScope<InfiniteGridPass>(device.get());

        CreateSwapchain();
        CreateFramebuffers();
        CreatePresentFramebuffers();
        CreateTransformsBuffer();

        CreateGBufferDescriptorSet();
        CreatePostProcessingDescriptorSet();

        CreateLightsBuffer();
        CreatePresentDescriptorSet();

        PrecomputeIBL();
    }

    void VulkanRenderer::PrecomputeIBL()
    {
        irradianceMap = IrradianceMapGenerator(device.get()).FromCubemapTexture(scene.skybox->GetCubemap());

        VkDescriptorImageInfo irradianceMapInfo{};
        irradianceMapInfo.sampler = irradianceMap->GetSampler();
        irradianceMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        irradianceMapInfo.imageView = irradianceMap->GetImageView();

        VulkanDescriptorWriter(*shaderCache->descriptorSetLayouts.irradianceDescriptorSetLayout, device->GetShaderDescriptorPool())
                .WriteImage(0, &irradianceMapInfo)
                .Build(shaderCache->descriptorSets.irradianceDescriptorSet);

        scene.reflectionProbe = ReflectionProbeGenerator(device.get()).FromCubemap(scene.skybox->GetCubemap());
    }

    void VulkanRenderer::DrawFrame(const float deltaTime, const Ref<Camera> camera)
    {
        UpdateTransformsBuffer(camera);
        UpdateLightsBuffer(deltaTime);
        scene.directionalLight.UpdateCascades(*camera);

        device->DrawFrame(vulkanSwapChain->GetSwapChain(), vulkanSwapChain->GetExtent(),
                          [&](const VkCommandBuffer commandBuffer, uint32_t, const uint32_t imageIndex) {
                              activeImage = imageIndex;

                              directionalShadowMap->PrepareToDepthRendering();
                              shadowMapPass->Render(commandBuffer, *camera, activeImage, framebuffers.shadowMapFramebuffers, nullptr);
                              directionalShadowMap->PrepareToShadowRendering();

                              gbufferPass->Render(commandBuffer, *camera, activeImage, framebuffers.gbufferFramebuffers);
                              ssaoPass->Render(commandBuffer, *camera, activeImage, framebuffers.ssaoFramebuffers);
                              skyboxPass->Render(commandBuffer, *camera, activeImage, framebuffers.geometryFramebuffers);
                              renderPass->Render(commandBuffer, *camera, activeImage, framebuffers.geometryFramebuffers);
                              gridPass->Render(commandBuffer, *camera, activeImage, framebuffers.geometryFramebuffers);
                              presentPass->Render(commandBuffer, *camera, activeImage, framebuffers.presentFramebuffers[activeImage]);
                          }, std::bind(&VulkanRenderer::ResizeSwapchain, this));
    }

    void VulkanRenderer::IdleWait()
    {
        vkDeviceWaitIdle(device->GetDevice());
    }

    void VulkanRenderer::Resize(const int width, const int height)
    {
        Renderer::Resize(width, height);

        viewportWidth = width;
        viewportHeight = height;

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
        vulkanSwapChain = VulkanSwapchain::Builder(device.get())
                .SetResolution(viewportWidth, viewportHeight)
                .SetPresentMode(VK_PRESENT_MODE_IMMEDIATE_KHR)
                .SetImageCount(imageCount)
                .SetImageFormat(surfaceFormat.format)
                .Build();
    }

    void VulkanRenderer::CreateGBufferDescriptorSet()
    {
        VkDescriptorImageInfo worldSpaceNormalInfo{};
        worldSpaceNormalInfo.sampler = framebuffers.gbufferFramebuffers->GetAttachments()[0].sampler;
        worldSpaceNormalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        worldSpaceNormalInfo.imageView = framebuffers.gbufferFramebuffers->GetAttachments()[0].imageView;

        VkDescriptorImageInfo positionInfo{};
        positionInfo.sampler = framebuffers.gbufferFramebuffers->GetAttachments()[1].sampler;
        positionInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        positionInfo.imageView = framebuffers.gbufferFramebuffers->GetAttachments()[1].imageView;

        VkDescriptorImageInfo depthInfo{};
        depthInfo.sampler = framebuffers.gbufferFramebuffers->GetAttachments()[2].sampler;
        depthInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        depthInfo.imageView = framebuffers.gbufferFramebuffers->GetAttachments()[2].imageView;

        VulkanDescriptorWriter(*ShaderCache::descriptorSetLayouts.gBufferDescriptorSetLayout, device->GetShaderDescriptorPool())
                .WriteImage(0, &worldSpaceNormalInfo)
                .WriteImage(1, &positionInfo)
                .WriteImage(2, &depthInfo)
                .Build(shaderCache->descriptorSets.gbufferDescriptorSet);
    }

    void VulkanRenderer::CreatePostProcessingDescriptorSet()
    {
        VkDescriptorImageInfo ssaoInfo{};
        ssaoInfo.sampler = framebuffers.ssaoFramebuffers->GetAttachments()[0].sampler;
        ssaoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        ssaoInfo.imageView = framebuffers.ssaoFramebuffers->GetAttachments()[0].imageView;

        VulkanDescriptorWriter(*ShaderCache::descriptorSetLayouts.postProcessingDescriptorSetLayout, device->GetShaderDescriptorPool())
                .WriteImage(0, &ssaoInfo)
                .Build(shaderCache->descriptorSets.postProcessingDescriptorSet);
    }

    void VulkanRenderer::CreatePresentDescriptorSet()
    {
        VkDescriptorImageInfo renderImageInfo{};
        renderImageInfo.sampler = framebuffers.geometryFramebuffers->GetAttachments()[0].sampler;
        renderImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        renderImageInfo.imageView = framebuffers.geometryFramebuffers->GetAttachments()[0].imageView;

        auto writer = VulkanDescriptorWriter(*shaderCache->descriptorSetLayouts.presentDescriptorSetLayout,
                                             device->GetShaderDescriptorPool());
        writer.WriteImage(0, &renderImageInfo);
        writer.Build(shaderCache->descriptorSets.presentDescriptorSet);
    }

    void VulkanRenderer::CreateFramebuffers()
    {
        const uint32_t RENDER_RESOLUTION_WIDTH = viewportWidth * resolutionScale;
        const uint32_t RENDER_RESOLUTION_HEIGHT = viewportHeight * resolutionScale;

        framebuffers.geometryFramebuffers = VulkanFramebuffer::Builder(device.get())
                .SetRenderpass(renderPass->GetRenderPass())
                .SetResolution(RENDER_RESOLUTION_WIDTH, RENDER_RESOLUTION_HEIGHT)
                .AddAttachment(ImageFormat::RGBA8_UNORM)
                .AddAttachment(ImageFormat::DEPTH24_STENCIL8)
                .Build();

        framebuffers.gbufferFramebuffers = VulkanFramebuffer::Builder(device.get())
                .SetRenderpass(gbufferPass->GetRenderPass())
                .SetResolution(RENDER_RESOLUTION_WIDTH, RENDER_RESOLUTION_HEIGHT)
                .AddAttachment(ImageFormat::RGBA32_SFLOAT)
                .AddAttachment(ImageFormat::RGBA32_SFLOAT)
                .AddAttachment(ImageFormat::DEPTH24_STENCIL8)
                .Build();

        framebuffers.ssaoFramebuffers = VulkanFramebuffer::Builder(device.get())
                .SetRenderpass(ssaoPass->GetRenderPass())
                .SetResolution(RENDER_RESOLUTION_WIDTH * 0.5, RENDER_RESOLUTION_HEIGHT * 0.5)
                .AddAttachment(ImageFormat::R8_UNORM)
                .Build();

        uint32_t SHADOW_MAP_RESOLUTION = 1024;
        directionalShadowMap = VulkanShadowMap::Builder()
                .SetResolution(SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION)
                .Build(device.get());

        framebuffers.shadowMapFramebuffers = VulkanFramebuffer::Builder(device.get())
                .SetRenderpass(shadowMapPass->GetRenderPass())
                .SetResolution(SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION)
                .AddAttachment(directionalShadowMap->GetImageView())
                .Build();
    }

    void VulkanRenderer::CreatePresentFramebuffers()
    {
        const uint32_t imageCount = VulkanUtils::GetSwapchainImageCount(device->GetPhysicalDevice(), device->GetSurface());
        framebuffers.presentFramebuffers.clear();
        framebuffers.presentFramebuffers.resize(imageCount);
        for (size_t i = 0; i < imageCount; i++)
        {
            framebuffers.presentFramebuffers[i] = VulkanFramebuffer::Builder(device.get())
                    .SetRenderpass(presentPass->GetRenderPass())
                    .SetResolution(viewportWidth, viewportHeight)
                    .AddAttachment(vulkanSwapChain->GetImageViews()[i])
                    .Build();
        }
    }

    void VulkanRenderer::ResizeSwapchain()
    {
        IdleWait();
        CreateSwapchain();
        CreatePresentFramebuffers();
    }

    void VulkanRenderer::CreateTransformsBuffer()
    {
        descriptorBuffers.transformsBuffer = CreateRef<VulkanBuffer>(
            device.get(),
            sizeof(TransformsBuffer),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU);

        VkDescriptorBufferInfo info{};
        info.buffer = descriptorBuffers.transformsBuffer->GetBuffer();
        info.offset = 0;
        info.range = sizeof(TransformsBuffer);

        VulkanDescriptorWriter(*shaderCache->descriptorSetLayouts.transformDescriptorSetLayout, device->GetShaderDescriptorPool())
                .WriteBuffer(0, &info)
                .Build(shaderCache->descriptorSets.transformDescriptorSet);
    }

    void VulkanRenderer::CreateLightsBuffer()
    {
        descriptorBuffers.lightsBuffer = CreateRef<VulkanBuffer>(
            device.get(),
            sizeof(LightsBuffer),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
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

        auto writer = VulkanDescriptorWriter(*shaderCache->descriptorSetLayouts.lightsDescriptorSetLayout,
                                             device->GetShaderDescriptorPool());
        writer.WriteBuffer(0, &info);
        writer.WriteImage(1, &shadowMapInfo);
        writer.Build(shaderCache->descriptorSets.lightsDescriptorSet);
    }

    void VulkanRenderer::UpdateTransformsBuffer(const Ref<Camera>& camera) const
    {
        TransformsBuffer bufferData;
        bufferData.cameraPosition = camera->GetTransform().m_Position;
        bufferData.view = camera->GetView();
        bufferData.proj = camera->GetProjection();

        memcpy(descriptorBuffers.transformsBuffer->GetMappedData(), &bufferData, sizeof(TransformsBuffer));
    }

    void VulkanRenderer::UpdateLightsBuffer(float deltaTime)
    {
        LightsBuffer bufferData;

        Transform lightTransform;
        lightTransform.m_Rotation = glm::vec3(45.0f, 0.0f, 0.0f);

        const glm::mat4 rot_mat = rotate(glm::mat4(1.0f), glm::radians(22.5f * deltaTime), glm::vec3(0.0, 1.0, 0.0));
        scene.directionalLight.direction = normalize(glm::vec3(glm::vec4(scene.directionalLight.direction, 1.0f) * rot_mat));

        bufferData.lightProjection = scene.directionalLight.cascades[2].viewProjMatrix;
        bufferData.lightView = scene.directionalLight.GetView();
        bufferData.direction = glm::vec4(normalize(scene.directionalLight.direction), 0.0);
        bufferData.color = glm::vec4(scene.directionalLight.color, 1.0f);
        bufferData.intensity = scene.directionalLight.intensity;
        bufferData.ambientIntensity = scene.directionalLight.ambientIntensity;
        bufferData.bias = scene.directionalLight.bias;

        memcpy(descriptorBuffers.lightsBuffer->GetMappedData(), &bufferData, sizeof(LightsBuffer));
    }
}
