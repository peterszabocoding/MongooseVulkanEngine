#include "vulkan_renderer.h"

#include <backends/imgui_impl_vulkan.h>

#include "imgui.h"
#include "vulkan_descriptor_writer.h"
#include "vulkan_framebuffer.h"
#include "resource/resource_manager.h"
#include "vulkan_pipeline.h"
#include "vulkan_device.h"
#include "vulkan_mesh.h"
#include "lighting/irradiance_map_generator.h"
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

        directionalLight.direction = normalize(glm::vec3(0.0f, -2.0f, -1.0f));

        cubeMesh = ResourceManager::LoadMesh(device.get(), "resources/models/cube.obj");
        screenRect = CreateScope<VulkanMeshlet>(device.get(), Primitives::RECTANGLE_VERTICES, Primitives::RECTANGLE_INDICES);

        LOG_TRACE("Load scene");
        scene = ResourceManager::LoadScene(device.get(), "resources/PBRCheck/pbr_check.gltf", "resources/environment/newport_loft.hdr");
        //completeScene = ResourceManager::LoadScene(vulkanDevice.get(), "resources/sponza/Sponza.gltf");
        //completeScene = ResourceManager::LoadScene(vulkanDevice.get(), "resources/MetalRoughSpheres/MetalRoughSpheres.gltf");
        //completeScene = ResourceManager::LoadScene(vulkanDevice.get(), "resources/vertex_color/vertex_color.gltf");
        //completeScene = ResourceManager::LoadScene(vulkanDevice.get(), "resources/normal_tangent/NormalTangentTest.gltf");
        //completeScene = ResourceManager::LoadScene(vulkanDevice.get(), "resources/gltf/multiple_spheres.gltf");
        //completeScene = ResourceManager::LoadScene(vulkanDevice.get(), "resources/chess/ABeautifulGame.gltf");
        //completeScene = ResourceManager::LoadScene(vulkanDevice.get(), "resources/SciFiHelmet/SciFiHelmet.gltf");
        //completeScene = ResourceManager::LoadScene(vulkanDevice.get(), "resources/DamagedHelmet/DamagedHelmet.gltf");
        //completeScene = ResourceManager::LoadScene(vulkanDevice.get(), "resources/tests/orientation_test/orientation_test.gltf");

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
        PrepareIrradianceMap();
        PrepareReflectionProbe();
    }

    void VulkanRenderer::PrepareIrradianceMap()
    {
        irradianceMap = IrradianceMapGenerator(device.get()).FromCubemapTexture(scene.skybox->GetCubemap());

        VkDescriptorImageInfo irradianceMapInfo{};
        irradianceMapInfo.sampler = irradianceMap->GetSampler();
        irradianceMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        irradianceMapInfo.imageView = irradianceMap->GetImageView();

        VulkanDescriptorWriter(*shaderCache->descriptorSetLayouts.irradianceDescriptorSetLayout, device->GetShaderDescriptorPool())
                .WriteImage(0, &irradianceMapInfo)
                .Build(shaderCache->descriptorSets.irradianceDescriptorSet);
    }

    void VulkanRenderer::PrepareReflectionProbe()
    {
        uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(128, 128)))) + 1;
        prefilterMap = VulkanCubeMapTexture::Builder()
                .SetFormat(ImageFormat::RGBA16_SFLOAT)
                .SetResolution(128, 128)
                .SetMipLevels(mipLevels)
                .Build(device.get());

        brdfLUT = VulkanTexture::Builder()
                .SetResolution(512, 512)
                .SetFormat(ImageFormat::RGBA16_SFLOAT)
                .AddAspectFlag(VK_IMAGE_ASPECT_COLOR_BIT)
                .AddUsage(VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
                .Build(device.get());

        auto iblBRDFFramebuffer = VulkanFramebuffer::Builder(device.get())
                .SetRenderpass(shaderCache->renderpasses.iblPreparePass)
                .SetResolution(512, 512)
                .AddAttachment(brdfLUT->GetImageView())
                .Build();

        constexpr uint32_t PREFILTER_MIP_LEVELS = 6;

        std::vector<std::vector<Ref<VulkanFramebuffer>>> iblPrefilterFramebuffes;
        iblPrefilterFramebuffes.resize(PREFILTER_MIP_LEVELS);

        for (unsigned int mip = 0; mip < PREFILTER_MIP_LEVELS; ++mip)
        {
            const unsigned int mipWidth = static_cast<unsigned int>(128 * std::pow(0.5, mip));
            const unsigned int mipHeight = static_cast<unsigned int>(128 * std::pow(0.5, mip));

            iblPrefilterFramebuffes[mip].resize(6);
            for (size_t i = 0; i < 6; i++)
            {
                iblPrefilterFramebuffes[mip][i] = VulkanFramebuffer::Builder(device.get())
                        .SetRenderpass(shaderCache->renderpasses.iblPreparePass)
                        .SetResolution(mipWidth, mipHeight)
                        .AddAttachment(prefilterMap->GetFaceImageView(i, mip))
                        .Build();
            }
        }

        device->ImmediateSubmit([&](VkCommandBuffer commandBuffer) {
            ComputeIblBRDF(commandBuffer, iblBRDFFramebuffer);

            for (unsigned int mip = 0; mip < PREFILTER_MIP_LEVELS; ++mip)
            {
                const float roughness = static_cast<float>(mip) / static_cast<float>(PREFILTER_MIP_LEVELS - 1);
                for (size_t i = 0; i < 6; i++)
                {
                    ComputePrefilterMap(commandBuffer, i, roughness, iblPrefilterFramebuffes[mip][i]);
                }
            }
        });


        VkDescriptorImageInfo prefilterMapInfo{};
        prefilterMapInfo.sampler = prefilterMap->GetSampler();
        prefilterMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        prefilterMapInfo.imageView = prefilterMap->GetImageView();

        VkDescriptorImageInfo brdfLUTInfo{};
        brdfLUTInfo.sampler = brdfLUT->GetSampler();
        brdfLUTInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        brdfLUTInfo.imageView = brdfLUT->GetImageView();

        VulkanDescriptorWriter(*shaderCache->descriptorSetLayouts.reflectionDescriptorSetLayout, device->GetShaderDescriptorPool())
                .WriteImage(0, &prefilterMapInfo)
                .WriteImage(1, &brdfLUTInfo)
                .Build(shaderCache->descriptorSets.reflectionDescriptorSet);
    }

    void VulkanRenderer::ComputeIblBRDF(VkCommandBuffer commandBuffer, Ref<VulkanFramebuffer> framebuffer)
    {
        VkExtent2D extent = {framebuffer->width, framebuffer->height};

        device->SetViewportAndScissor(extent, commandBuffer);
        shaderCache->renderpasses.iblPreparePass->Begin(commandBuffer, framebuffer, extent);

        DrawCommandParams drawCommandParams{};
        drawCommandParams.commandBuffer = commandBuffer;

        drawCommandParams.pipelineParams =
        {
            shaderCache->pipelines.ibl_brdf->GetPipeline(),
            shaderCache->pipelines.ibl_brdf->GetPipelineLayout()
        };

        drawCommandParams.meshlet = screenRect.get();

        device->DrawMeshlet(drawCommandParams);
    }

    void VulkanRenderer::ComputePrefilterMap(VkCommandBuffer commandBuffer, size_t faceIndex, float roughness,
                                             Ref<VulkanFramebuffer> framebuffer)
    {
        VkExtent2D extent = {
            framebuffer->width,
            framebuffer->height
        };

        device->SetViewportAndScissor(extent, commandBuffer);
        shaderCache->renderpasses.iblPreparePass->Begin(commandBuffer, framebuffer, extent);

        DrawCommandParams drawCommandParams{};
        drawCommandParams.commandBuffer = commandBuffer;

        drawCommandParams.pipelineParams = {
            shaderCache->pipelines.ibl_prefilter->GetPipeline(),
            shaderCache->pipelines.ibl_prefilter->GetPipelineLayout()
        };

        drawCommandParams.meshlet = &cubeMesh->GetMeshlets()[0];

        drawCommandParams.descriptorSets = {
            scene.skybox->descriptorSet,
        };

        PrefilterData pushConstantData;
        pushConstantData.projection = m_CaptureProjection;
        pushConstantData.view = m_CaptureViews[faceIndex];
        pushConstantData.roughness = roughness;

        drawCommandParams.pushConstantParams = {
            &pushConstantData,
            sizeof(PrefilterData)
        };

        device->DrawMeshlet(drawCommandParams);

        shaderCache->renderpasses.iblPreparePass->End(commandBuffer);
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

                              renderPass->SetCamera(*camera);
                              renderPass->SetSize(
                                  framebuffers.geometryFramebuffers[activeImage]->width,
                                  framebuffers.geometryFramebuffers[activeImage]->height);
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
        directionalLight.direction = normalize(glm::vec3(glm::vec4(directionalLight.direction, 1.0f) * rot_mat));

        bufferData.lightProjection = directionalLight.GetProjection();
        bufferData.lightView = directionalLight.GetView();
        bufferData.direction = glm::vec4(normalize(directionalLight.direction), 0.0);
        bufferData.color = glm::vec4(directionalLight.color, 1.0f);
        bufferData.intensity = directionalLight.intensity;
        bufferData.ambientIntensity = directionalLight.ambientIntensity;
        bufferData.bias = directionalLight.bias;

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
