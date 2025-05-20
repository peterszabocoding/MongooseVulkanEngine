#include "vulkan_renderer.h"

#include "imgui.h"
#include "vulkan_descriptor_writer.h"
#include "vulkan_framebuffer.h"
#include "resource/resource_manager.h"
#include "vulkan_pipeline.h"
#include "vulkan_device.h"
#include "vulkan_mesh.h"
#include "lighting/irradiance_map_generator.h"
#include "lighting/reflection_probe_generator.h"
#include "util/log.h"

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
        //scene = ResourceManager::LoadScene(device.get(), "resources/PBRCheck/pbr_check.gltf", "resources/environment/newport_loft.hdr");
        scene = ResourceManager::LoadScene(device.get(), "resources/cannon/cannon.gltf", "resources/environment/newport_loft.hdr");
        //completeScene = ResourceManager::LoadScene(vulkanDevice.get(), "resources/sponza/Sponza.gltf");
        //completeScene = ResourceManager::LoadScene(vulkanDevice.get(), "resources/MetalRoughSpheres/MetalRoughSpheres.gltf");
        //completeScene = ResourceManager::LoadScene(vulkanDevice.get(), "resources/vertex_color/vertex_color.gltf");
        //completeScene = ResourceManager::LoadScene(vulkanDevice.get(), "resources/normal_tangent/NormalTangentTest.gltf");
        //completeScene = ResourceManager::LoadScene(vulkanDevice.get(), "resources/gltf/multiple_spheres.gltf");
        //completeScene = ResourceManager::LoadScene(vulkanDevice.get(), "resources/chess/ABeautifulGame.gltf");
        //completeScene = ResourceManager::LoadScene(vulkanDevice.get(), "resources/SciFiHelmet/SciFiHelmet.gltf");
        //completeScene = ResourceManager::LoadScene(vulkanDevice.get(), "resources/DamagedHelmet/DamagedHelmet.gltf");
        //completeScene = ResourceManager::LoadScene(vulkanDevice.get(), "resources/tests/orientation_test/orientation_test.gltf");
        scene.directionalLight.direction = normalize(glm::vec3(0.0f, -2.0f, -1.0f));

        gbufferPass = CreateScope<GBufferPass>(device.get(), scene);
        renderPass = CreateScope<RenderPass>(device.get(), scene);
        shadowMapPass = CreateScope<ShadowMapPass>(device.get(), scene);
        presentPass = CreateScope<PresentPass>(device.get());

        CreateSwapchain();
        CreateFramebuffers();
        CreateTransformsBuffer();

        CreateLightsBuffer();
        PreparePresentPass();

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

        device->DrawFrame(vulkanSwapChain->GetSwapChain(), vulkanSwapChain->GetExtent(),
                          [&](const VkCommandBuffer commandBuffer, uint32_t, const uint32_t imageIndex) {
                              activeImage = imageIndex;

                              framebuffers.directionalShadowMaps[activeImage]->PrepareToDepthRendering();
                              shadowMapPass->Render(commandBuffer, activeImage, framebuffers.shadowMapFramebuffers[activeImage], nullptr);
                              framebuffers.directionalShadowMaps[activeImage]->PrepareToShadowRendering();

                              gbufferPass->SetCamera(*camera);
                              gbufferPass->SetSize(
                                  framebuffers.prepassFramebuffers[activeImage]->GetWidth(),
                                  framebuffers.prepassFramebuffers[activeImage]->GetHeight()
                              );
                              gbufferPass->Render(commandBuffer, activeImage, framebuffers.prepassFramebuffers[activeImage], nullptr);

                              renderPass->SetCamera(*camera);
                              renderPass->SetSize(
                                  framebuffers.geometryFramebuffers[activeImage]->GetWidth(),
                                  framebuffers.geometryFramebuffers[activeImage]->GetHeight());
                              renderPass->Render(commandBuffer, activeImage, framebuffers.geometryFramebuffers[activeImage], nullptr);

                              presentPass->SetSize(vulkanSwapChain->GetExtent().width, vulkanSwapChain->GetExtent().height);
                              presentPass->Render(commandBuffer, activeImage, framebuffers.presentFramebuffers[activeImage], nullptr);
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

    void VulkanRenderer::CreateFramebuffers()
    {
        const uint32_t imageCount = VulkanUtils::GetSwapchainImageCount(device->GetPhysicalDevice(), device->GetSurface());

        framebuffers.prepassFramebuffers.resize(imageCount);
        framebuffers.geometryFramebuffers.resize(imageCount);
        framebuffers.shadowMapFramebuffers.resize(imageCount);
        framebuffers.directionalShadowMaps.resize(imageCount);
        framebuffers.presentFramebuffers.resize(imageCount);

        for (size_t i = 0; i < imageCount; i++)
        {
            framebuffers.geometryFramebuffers[i] = VulkanFramebuffer::Builder(device.get())
                    .SetRenderpass(renderPass->GetRenderPass())
                    .SetResolution(viewportWidth * resolutionScale, viewportHeight * resolutionScale)
                    .AddAttachment(ImageFormat::RGBA8_UNORM)
                    .AddAttachment(ImageFormat::RGBA8_UNORM)
                    .AddAttachment(ImageFormat::RGBA8_UNORM)
                    .AddAttachment(ImageFormat::RGBA8_UNORM)
                    .AddAttachment(ImageFormat::RGBA8_UNORM)
                    .AddAttachment(ImageFormat::DEPTH24_STENCIL8)
                    .Build();

            framebuffers.prepassFramebuffers[i] = VulkanFramebuffer::Builder(device.get())
                    .SetRenderpass(renderPass->GetRenderPass())
                    .SetResolution(viewportWidth * resolutionScale, viewportHeight * resolutionScale)
                    .AddAttachment(ImageFormat::RGBA32_SFLOAT)
                    .AddAttachment(ImageFormat::RGBA32_SFLOAT)
                    .AddAttachment(ImageFormat::DEPTH24_STENCIL8)
                    .Build();

            framebuffers.directionalShadowMaps[i] = VulkanShadowMap::Builder()
                    .SetResolution(4096, 4096)
                    .Build(device.get());

            framebuffers.shadowMapFramebuffers[i] = VulkanFramebuffer::Builder(device.get())
                    .SetRenderpass(shadowMapPass->GetRenderPass())
                    .SetResolution(4096, 4096)
                    .AddAttachment(framebuffers.directionalShadowMaps[i]->GetImageView())
                    .Build();

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

        framebuffers.geometryFramebuffers.clear();
        framebuffers.presentFramebuffers.clear();

        CreateSwapchain();
        CreateFramebuffers();
        CreateLightsBuffer();
        PreparePresentPass();
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
        if (!descriptorBuffers.lightsBuffer)
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

            const uint32_t imageCount = VulkanUtils::GetSwapchainImageCount(device->GetPhysicalDevice(), device->GetSurface());
            shaderCache->descriptorSets.lightsDescriptorSets.resize(imageCount);

            for (size_t i = 0; i < imageCount; i++)
            {
                VkDescriptorImageInfo shadowMapInfo{};
                shadowMapInfo.sampler = framebuffers.directionalShadowMaps[i]->GetSampler();
                shadowMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                shadowMapInfo.imageView = framebuffers.directionalShadowMaps[i]->GetImageView();

                VulkanDescriptorWriter(*shaderCache->descriptorSetLayouts.lightsDescriptorSetLayout,
                                       device->GetShaderDescriptorPool())
                        .WriteBuffer(0, &info)
                        .WriteImage(1, &shadowMapInfo)
                        .Build(shaderCache->descriptorSets.lightsDescriptorSets[i]);
            }
        } else
        {
            const uint32_t imageCount = VulkanUtils::GetSwapchainImageCount(device->GetPhysicalDevice(), device->GetSurface());
            shaderCache->descriptorSets.lightsDescriptorSets.resize(imageCount);

            for (size_t i = 0; i < imageCount; i++)
            {
                VkDescriptorImageInfo shadowMapInfo{};
                shadowMapInfo.sampler = framebuffers.directionalShadowMaps[i]->GetSampler();
                shadowMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                shadowMapInfo.imageView = framebuffers.directionalShadowMaps[i]->GetImageView();

                VulkanDescriptorWriter(*shaderCache->descriptorSetLayouts.lightsDescriptorSetLayout,
                                       device->GetShaderDescriptorPool())
                        .WriteImage(1, &shadowMapInfo)
                        .Overwrite(shaderCache->descriptorSets.lightsDescriptorSets[i]);
            }
        }
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

        bufferData.lightProjection = scene.directionalLight.GetProjection();
        bufferData.lightView = scene.directionalLight.GetView();
        bufferData.direction = glm::vec4(normalize(scene.directionalLight.direction), 0.0);
        bufferData.color = glm::vec4(scene.directionalLight.color, 1.0f);
        bufferData.intensity = scene.directionalLight.intensity;
        bufferData.ambientIntensity = scene.directionalLight.ambientIntensity;
        bufferData.bias = scene.directionalLight.bias;

        memcpy(descriptorBuffers.lightsBuffer->GetMappedData(), &bufferData, sizeof(LightsBuffer));
    }

    void VulkanRenderer::PreparePresentPass()
    {
        const uint32_t imageCount = VulkanUtils::GetSwapchainImageCount(device->GetPhysicalDevice(), device->GetSurface());
        shaderCache->descriptorSets.presentDescriptorSets.clear();
        shaderCache->descriptorSets.presentDescriptorSets.resize(imageCount);

        for (size_t i = 0; i < imageCount; i++)
        {
            VkDescriptorImageInfo renderImageInfo{};
            renderImageInfo.sampler = framebuffers.geometryFramebuffers[i]->GetAttachments()[0].sampler;
            renderImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            renderImageInfo.imageView = framebuffers.geometryFramebuffers[i]->GetAttachments()[0].imageView;

            auto writer = VulkanDescriptorWriter(*shaderCache->descriptorSetLayouts.presentDescriptorSetLayout,
                                                 device->GetShaderDescriptorPool())
                    .WriteImage(0, &renderImageInfo);

            if (!shaderCache->descriptorSets.presentDescriptorSets[i])
                writer.Build(shaderCache->descriptorSets.presentDescriptorSets[i]);
            else
                writer.Overwrite(shaderCache->descriptorSets.presentDescriptorSets[i]);
        }
    }
}
