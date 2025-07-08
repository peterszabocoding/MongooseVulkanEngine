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
#include "renderer/render_graph.h"
#include "util/log.h"
#include "util/filesystem.h"

namespace Raytracing
{
    VulkanRenderer::~VulkanRenderer() {}

    void VulkanRenderer::Init(const uint32_t width, const uint32_t height)
    {
        LOG_TRACE("VulkanRenderer::Init()");
        viewportResolution.width = width;
        viewportResolution.height = height;

        renderResolution.width = resolutionScale * viewportResolution.width;
        renderResolution.height = resolutionScale * viewportResolution.height;

        device = CreateScope<VulkanDevice>(width, height, glfwWindow);
        shaderCache = CreateScope<ShaderCache>(device.get());

        cubeMesh = ResourceManager::LoadMesh(device.get(), "resources/models/cube.obj");
        screenRect = CreateScope<VulkanMeshlet>(device.get(), Primitives::RECTANGLE_VERTICES, Primitives::RECTANGLE_INDICES);

        LOG_TRACE("Load scene");
        scene = ResourceManager::LoadScene(device.get(), "resources/sponza/Sponza.gltf", "resources/environment/etzwihl_4k.hdr");
        //scene = ResourceManager::LoadScene(device.get(), "resources/PBRCheck/pbr_check.gltf", "resources/environment/etzwihl_4k.hdr");
        //scene = ResourceManager::LoadScene(device.get(), "resources/cannon/cannon.gltf", "resources/environment/etzwihl_4k.hdr");
        //scene = ResourceManager::LoadScene(device.get(), "resources/MetalRoughSpheres/MetalRoughSpheres.gltf", "resources/environment/etzwihl_4k.hdr");
        //scene = ResourceManager::LoadScene(device.get(), "resources/gltf/multiple_spheres.gltf", "resources/environment/etzwihl_4k.hdr");
        //scene = ResourceManager::LoadScene(device.get(), "resources/chess/ABeautifulGame.gltf", "resources/environment/etzwihl_4k.hdr");
        //scene = ResourceManager::LoadScene(device.get(), "resources/DamagedHelmet/DamagedHelmet.gltf", "resources/environment/etzwihl_4k.hdr");

        scene.directionalLight.direction = normalize(glm::vec3(0.0f, -2.0f, -1.0f));

        gbufferPass = CreateScope<GBufferPass>(device.get(), scene, renderResolution);
        skyboxPass = CreateScope<SkyboxPass>(device.get(), scene, renderResolution);
        renderPass = CreateScope<RenderPass>(device.get(), scene, renderResolution);
        shadowMapPass = CreateScope<ShadowMapPass>(device.get(), scene, renderResolution);
        presentPass = CreateScope<PresentPass>(device.get(), renderResolution);
        ssaoPass = CreateScope<SSAOPass>(device.get(), renderResolution);
        gridPass = CreateScope<InfiniteGridPass>(device.get(), renderResolution);

        CreateSwapchain();

        CreateShadowMap();
        CreateFramebuffers();
        CreateTransformsBuffer();

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

    void VulkanRenderer::Draw(const float deltaTime, const Ref<Camera> camera)
    {
        device->DrawFrame(vulkanSwapChain->GetSwapChain(),
                          [&](const VkCommandBuffer cmd, const uint32_t imgIndex) {
                              RotateLight(deltaTime);
                              scene.directionalLight.UpdateCascades(*camera);

                              UpdateLightsBuffer(deltaTime);
                              UpdateTransformsBuffer(camera);
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
        Renderer::Resize(width, height);

        viewportResolution.width = width;
        viewportResolution.height = height;

        renderResolution.width = resolutionScale * viewportResolution.width;
        renderResolution.height = resolutionScale * viewportResolution.height;

        gbufferPass->Resize(renderResolution);
        skyboxPass->Resize(renderResolution);
        renderPass->Resize(renderResolution);
        shadowMapPass->Resize(renderResolution);
        ssaoPass->Resize(renderResolution);
        gridPass->Resize(renderResolution);

        presentPass->Resize(viewportResolution);

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
                .Build(device.get());

        framebuffers.shadowMapFramebuffers.resize(SHADOW_MAP_CASCADE_COUNT);
        for (size_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
        {
            framebuffers.shadowMapFramebuffers[i] = VulkanFramebuffer::Builder(device.get())
                    .SetRenderpass(shadowMapPass->GetRenderPass())
                    .SetResolution(SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION)
                    .AddAttachment(directionalShadowMap->GetImageView(i))
                    .Build();
        }
    }

    void VulkanRenderer::CreateFramebuffers()
    {
        framebuffers.geometryFramebuffer = VulkanFramebuffer::Builder(device.get())
                .SetRenderpass(renderPass->GetRenderPass())
                .SetResolution(renderResolution.width, renderResolution.height)
                .AddAttachment(ImageFormat::RGBA16_SFLOAT)
                .AddAttachment(ImageFormat::DEPTH24_STENCIL8).Build();

        const uint32_t imageCount = VulkanUtils::GetSwapchainImageCount(device->GetPhysicalDevice(), device->GetSurface());
        framebuffers.presentFramebuffers.clear();
        framebuffers.presentFramebuffers.resize(imageCount);
        for (size_t i = 0; i < imageCount; i++)
        {
            framebuffers.presentFramebuffers[i] = VulkanFramebuffer::Builder(device.get())
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
        CreateFramebuffers();
        CreatePresentDescriptorSet();
    }

    void VulkanRenderer::CreateTransformsBuffer()
    {
        descriptorBuffers.transformsBuffer = CreateRef<VulkanBuffer>(device.get(), sizeof(TransformsBuffer),
                                                                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                                                                     VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        VkDescriptorBufferInfo info{};
        info.buffer = descriptorBuffers.transformsBuffer->GetBuffer();
        info.offset = 0;
        info.range = sizeof(TransformsBuffer);

        VulkanDescriptorWriter(*shaderCache->descriptorSetLayouts.transformDescriptorSetLayout, device->GetShaderDescriptorPool()).
                WriteBuffer(0, &info).Build(shaderCache->descriptorSets.transformDescriptorSet);
    }

    void VulkanRenderer::CreateLightsBuffer()
    {
        descriptorBuffers.lightsBuffer = CreateRef<VulkanBuffer>(device.get(), sizeof(LightsBuffer),
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

        memcpy(descriptorBuffers.transformsBuffer->GetData(), &bufferData, sizeof(TransformsBuffer));
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

    void VulkanRenderer::DrawFrame(const VkCommandBuffer commandBuffer, const uint32_t imageIndex, const Ref<Camera>& camera)
    {
        activeImage = imageIndex;

        gbufferPass->Render(commandBuffer, *camera, nullptr);
        directionalShadowMap->TransitionToDepthRendering(commandBuffer);
        for (size_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
        {
            shadowMapPass->SetCascadeIndex(i);
            shadowMapPass->Render(commandBuffer, *camera, framebuffers.shadowMapFramebuffers[i]);
        }
        directionalShadowMap->TransitionToShaderRead(commandBuffer);

        ssaoPass->Render(commandBuffer, *camera, nullptr);
        skyboxPass->Render(commandBuffer, *camera, framebuffers.geometryFramebuffer);

        renderPass->Render(commandBuffer, *camera, framebuffers.geometryFramebuffer);
        gridPass->Render(commandBuffer, *camera, framebuffers.geometryFramebuffer);

        presentPass->Render(commandBuffer, *camera, framebuffers.presentFramebuffers[activeImage]);
    }
}
