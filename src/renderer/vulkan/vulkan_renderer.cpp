#include "vulkan_renderer.h"

#include <backends/imgui_impl_vulkan.h>

#include "imgui.h"
#include "vulkan_descriptor_writer.h"
#include "vulkan_framebuffer.h"
#include "resource/resource_manager.h"
#include "vulkan_pipeline.h"
#include "vulkan_device.h"
#include "vulkan_mesh.h"
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
        viewportWidth = width;
        viewportHeight = height;

        LOG_TRACE("VulkanRenderer::Init()");
        vulkanDevice = CreateScope<VulkanDevice>(width, height, glfwWindow);
        shaderCache = CreateScope<ShaderCache>(vulkanDevice.get());

        CreateSwapchain();
        shaderCache->Load();

        CreateFramebuffers();
        CreateTransformsBuffer();

        directionalLight.direction = normalize(glm::vec3(0.0f, -2.0f, -1.0f));
        CreateLightsBuffer();

        LOG_TRACE("Load skybox");
        cubeMesh = ResourceManager::LoadMesh(vulkanDevice.get(), "resources/models/cube.obj");
        skyboxTexture = ResourceManager::LoadHDRCubeMap(vulkanDevice.get(), "resources/environment/newport_loft.hdr");
        //skyboxTexture = ResourceManager::LoadHDRCubeMap(vulkanDevice.get(), "resources/environment/kloppenheim_06_puresky_4k.hdr");
        UploadCubemapTexture(skyboxTexture);

        screenRect = CreateScope<VulkanMeshlet>(vulkanDevice.get(), Primitives::RECTANGLE_VERTICES, Primitives::RECTANGLE_INDICES);
        PreparePresentPass();

        LOG_TRACE("Load scene");
        //completeScene = ResourceManager::LoadScene(vulkanDevice.get(), "resources/sponza/Sponza.gltf");
        //completeScene = ResourceManager::LoadScene(vulkanDevice.get(), "resources/MetalRoughSpheres/MetalRoughSpheres.gltf");
        completeScene = ResourceManager::LoadScene(vulkanDevice.get(), "resources/PBRCheck/pbr_check.gltf");
        //completeScene = ResourceManager::LoadScene(vulkanDevice.get(), "resources/vertex_color/vertex_color.gltf");
        //completeScene = ResourceManager::LoadScene(vulkanDevice.get(), "resources/normal_tangent/NormalTangentTest.gltf");
        //completeScene = ResourceManager::LoadScene(vulkanDevice.get(), "resources/gltf/multiple_spheres.gltf");
        //completeScene = ResourceManager::LoadScene(vulkanDevice.get(), "resources/chess/ABeautifulGame.gltf");
        //completeScene = ResourceManager::LoadScene(vulkanDevice.get(), "resources/tests/orientation_test/orientation_test.gltf");

        PrecomputeIBL();
    }

    void VulkanRenderer::PrecomputeIBL()
    {
        irradianceMap = VulkanCubeMapTexture::Builder()
                .SetFormat(ImageFormat::RGBA16_SFLOAT)
                .SetResolution(32, 32)
                .Build(vulkanDevice.get());

        uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(128, 128)))) + 1;
        prefilterMap = VulkanCubeMapTexture::Builder()
                .SetFormat(ImageFormat::RGBA16_SFLOAT)
                .SetResolution(128, 128)
                .SetMipLevels(mipLevels)
                .Build(vulkanDevice.get());

        framebuffers.iblBRDFFramebuffer = VulkanFramebuffer::Builder(vulkanDevice.get())
                .SetRenderpass(shaderCache->renderpasses.iblPreparePass)
                .SetResolution(512, 512)
                .AddAttachment(ImageFormat::RGBA16_SFLOAT)
                .Build();

        framebuffers.iblIrradianceFramebuffes.resize(6);
        for (size_t i = 0; i < 6; i++)
        {
            framebuffers.iblIrradianceFramebuffes[i] = VulkanFramebuffer::Builder(vulkanDevice.get())
                    .SetRenderpass(shaderCache->renderpasses.iblPreparePass)
                    .SetResolution(32, 32)
                    .AddAttachment(irradianceMap->GetFaceImageView(i, 0))
                    .Build();
        }

        constexpr uint32_t PREFILTER_MIP_LEVELS = 6;

        framebuffers.iblPrefilterFramebuffes.resize(PREFILTER_MIP_LEVELS);
        for (unsigned int mip = 0; mip < PREFILTER_MIP_LEVELS; ++mip)
        {
            const unsigned int mipWidth = static_cast<unsigned int>(128 * std::pow(0.5, mip));
            const unsigned int mipHeight = static_cast<unsigned int>(128 * std::pow(0.5, mip));

            framebuffers.iblPrefilterFramebuffes[mip].resize(6);
            for (size_t i = 0; i < 6; i++)
            {
                framebuffers.iblPrefilterFramebuffes[mip][i] = VulkanFramebuffer::Builder(vulkanDevice.get())
                        .SetRenderpass(shaderCache->renderpasses.iblPreparePass)
                        .SetResolution(mipWidth, mipHeight)
                        .AddAttachment(prefilterMap->GetFaceImageView(i, mip))
                        .Build();
            }
        }

        vulkanDevice->DrawFrame(vulkanSwapChain->GetSwapChain(), vulkanSwapChain->GetExtent(),
                                [&](const VkCommandBuffer commandBuffer, uint32_t, const uint32_t imageIndex) {
                                    activeImage = imageIndex;

                                    ComputeIblBRDF(commandBuffer);

                                    for (size_t i = 0; i < 6; i++)
                                    {
                                        ComputeIrradianceMap(commandBuffer, i);
                                    }


                                    for (unsigned int mip = 0; mip < PREFILTER_MIP_LEVELS; ++mip)
                                    {
                                        const float roughness = static_cast<float>(mip) / static_cast<float>(PREFILTER_MIP_LEVELS - 1);
                                        for (size_t i = 0; i < 6; i++)
                                        {
                                            ComputePrefilterMap(commandBuffer, i, mip, roughness);
                                        }
                                    }


                                    shaderCache->renderpasses.iblPreparePass->End(commandBuffer);
                                }, std::bind(&VulkanRenderer::ResizeSwapchain, this));

        PreparePBR();
    }

    void VulkanRenderer::ComputeIblBRDF(VkCommandBuffer commandBuffer)
    {
        VkExtent2D extent = {framebuffers.iblBRDFFramebuffer->width, framebuffers.iblBRDFFramebuffer->height};

        vulkanDevice->SetViewportAndScissor(extent, commandBuffer);
        shaderCache->renderpasses.iblPreparePass->Begin(commandBuffer, framebuffers.iblBRDFFramebuffer, extent);

        DrawCommandParams drawCommandParams{};
        drawCommandParams.commandBuffer = commandBuffer;
        drawCommandParams.pipeline = shaderCache->pipelines.ibl_brdf->GetPipeline();
        drawCommandParams.pipelineLayout = shaderCache->pipelines.ibl_brdf->GetPipelineLayout();
        drawCommandParams.meshlet = screenRect.get();

        vulkanDevice->DrawMeshlet(drawCommandParams);
    }

    void VulkanRenderer::ComputeIrradianceMap(VkCommandBuffer commandBuffer, size_t faceIndex)
    {
        VkExtent2D extent = {
            framebuffers.iblIrradianceFramebuffes[faceIndex]->width,
            framebuffers.iblIrradianceFramebuffes[faceIndex]->height
        };

        vulkanDevice->SetViewportAndScissor(extent, commandBuffer);
        shaderCache->renderpasses.iblPreparePass->Begin(commandBuffer, framebuffers.iblIrradianceFramebuffes[faceIndex], extent);

        DrawCommandParams drawCommandParams{};
        drawCommandParams.commandBuffer = commandBuffer;
        drawCommandParams.pipeline = shaderCache->pipelines.ibl_irradianceMap->GetPipeline();
        drawCommandParams.pipelineLayout = shaderCache->pipelines.ibl_irradianceMap->GetPipelineLayout();
        drawCommandParams.meshlet = &cubeMesh->GetMeshlets()[0];

        drawCommandParams.descriptorSets = {
            shaderCache->descriptorSets.cubemapDescriptorSet,
        };

        TransformPushConstantData pushConstantData;
        pushConstantData.projection = m_CaptureProjection;
        pushConstantData.view = m_CaptureViews[faceIndex];

        drawCommandParams.pushConstantData = &pushConstantData;
        drawCommandParams.pushConstantSize = sizeof(TransformPushConstantData);
        drawCommandParams.pushConstantShaderStageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        vulkanDevice->DrawMeshlet(drawCommandParams);
    }

    void VulkanRenderer::ComputePrefilterMap(VkCommandBuffer commandBuffer, size_t faceIndex, uint32_t miplevel, float roughness)
    {
        VkExtent2D extent = {
            framebuffers.iblPrefilterFramebuffes[miplevel][faceIndex]->width,
            framebuffers.iblPrefilterFramebuffes[miplevel][faceIndex]->height
        };

        vulkanDevice->SetViewportAndScissor(extent, commandBuffer);
        shaderCache->renderpasses.iblPreparePass->Begin(commandBuffer, framebuffers.iblPrefilterFramebuffes[miplevel][faceIndex], extent);

        DrawCommandParams drawCommandParams{};
        drawCommandParams.commandBuffer = commandBuffer;
        drawCommandParams.pipeline = shaderCache->pipelines.ibl_prefilter->GetPipeline();
        drawCommandParams.pipelineLayout = shaderCache->pipelines.ibl_prefilter->GetPipelineLayout();
        drawCommandParams.meshlet = &cubeMesh->GetMeshlets()[0];

        drawCommandParams.descriptorSets = {
            shaderCache->descriptorSets.cubemapDescriptorSet,
        };

        PrefilterData pushConstantData;
        pushConstantData.projection = m_CaptureProjection;
        pushConstantData.view = m_CaptureViews[faceIndex];
        pushConstantData.roughness = roughness;

        drawCommandParams.pushConstantData = &pushConstantData;
        drawCommandParams.pushConstantSize = sizeof(PrefilterData);
        drawCommandParams.pushConstantShaderStageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        vulkanDevice->DrawMeshlet(drawCommandParams);
    }

    void VulkanRenderer::DrawFrame(const float deltaTime, const Ref<Camera> camera)
    {
        UpdateTransformsBuffer(camera);
        UpdateLightsBuffer(deltaTime);

        vulkanDevice->DrawFrame(vulkanSwapChain->GetSwapChain(), vulkanSwapChain->GetExtent(),
                                [&](const VkCommandBuffer commandBuffer, uint32_t, const uint32_t imageIndex) {
                                    activeImage = imageIndex;

                                    framebuffers.directionalShadowMaps[activeImage]->PrepareToDepthRendering();
                                    DrawDirectionalShadowMapPass(commandBuffer);
                                    framebuffers.directionalShadowMaps[activeImage]->PrepareToShadowRendering();

                                    DrawGeometryPass(camera, commandBuffer);
                                    DrawUIPass(commandBuffer);
                                }, std::bind(&VulkanRenderer::ResizeSwapchain, this));
    }

    void VulkanRenderer::DrawSkybox(const VkCommandBuffer commandBuffer) const
    {
        DrawCommandParams skyboxDrawParams{};
        skyboxDrawParams.commandBuffer = commandBuffer;
        skyboxDrawParams.meshlet = &cubeMesh->GetMeshlets()[0];
        skyboxDrawParams.pipeline = shaderCache->pipelines.skyBox->GetPipeline();
        skyboxDrawParams.pipelineLayout = shaderCache->pipelines.skyBox->GetPipelineLayout();
        skyboxDrawParams.descriptorSets = {
            shaderCache->descriptorSets.cubemapDescriptorSet, shaderCache->descriptorSets.transformDescriptorSet
        };

        vulkanDevice->DrawMeshlet(skyboxDrawParams);
    }

    void VulkanRenderer::DrawDirectionalShadowMapPass(const VkCommandBuffer commandBuffer)
    {
        VkExtent2D extent = {4096, 4096};

        vulkanDevice->SetViewportAndScissor(extent, commandBuffer);
        shaderCache->renderpasses.shadowMapPass->Begin(commandBuffer, framebuffers.shadowMapFramebuffers[activeImage], extent);

        DrawCommandParams geometryDrawParams{};
        geometryDrawParams.commandBuffer = commandBuffer;
        geometryDrawParams.pipeline = shaderCache->pipelines.directionalShadowMap->GetPipeline();
        geometryDrawParams.pipelineLayout = shaderCache->pipelines.directionalShadowMap->GetPipelineLayout();

        for (size_t i = 0; i < completeScene.meshes.size(); i++)
        {
            SimplePushConstantData pushConstantData;
            pushConstantData.modelMatrix = completeScene.transforms[i].GetTransform();

            geometryDrawParams.pushConstantData = &pushConstantData;
            geometryDrawParams.pushConstantSize = sizeof(SimplePushConstantData);
            geometryDrawParams.pushConstantShaderStageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

            for (auto& meshlet: completeScene.meshes[i]->GetMeshlets())
            {
                geometryDrawParams.descriptorSets = {
                    shaderCache->descriptorSets.lightsDescriptorSets[activeImage]
                };
                geometryDrawParams.meshlet = &meshlet;
                vulkanDevice->DrawMeshlet(geometryDrawParams);
            }
        }

        shaderCache->renderpasses.shadowMapPass->End(commandBuffer);
    }

    void VulkanRenderer::DrawGeometryPass(const Ref<Camera>& camera, const VkCommandBuffer commandBuffer) const
    {
        VkExtent2D extent = {framebuffers.geometryFramebuffers[activeImage]->width, framebuffers.geometryFramebuffers[activeImage]->height};

        vulkanDevice->SetViewportAndScissor(extent, commandBuffer);
        shaderCache->renderpasses.geometryPass->Begin(commandBuffer, framebuffers.geometryFramebuffers[activeImage], extent);

        DrawSkybox(commandBuffer);

        DrawCommandParams geometryDrawParams{};
        geometryDrawParams.commandBuffer = commandBuffer;
        geometryDrawParams.pipeline = shaderCache->pipelines.geometry->GetPipeline();
        geometryDrawParams.pipelineLayout = shaderCache->pipelines.geometry->GetPipelineLayout();

        for (size_t i = 0; i < completeScene.meshes.size(); i++)
        {
            SimplePushConstantData pushConstantData;
            pushConstantData.modelMatrix = completeScene.transforms[i].GetTransform();
            pushConstantData.transform = camera->GetProjection() * camera->GetView() * pushConstantData.modelMatrix;

            auto& materials = completeScene.meshes[i]->GetMaterials();

            geometryDrawParams.pushConstantData = &pushConstantData;
            geometryDrawParams.pushConstantSize = sizeof(SimplePushConstantData);
            geometryDrawParams.pushConstantShaderStageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

            for (auto& meshlet: completeScene.meshes[i]->GetMeshlets())
            {
                geometryDrawParams.descriptorSets = {
                    materials[meshlet.GetMaterialIndex()].descriptorSet,
                    shaderCache->descriptorSets.transformDescriptorSet,
                    shaderCache->descriptorSets.cubemapDescriptorSet,
                    shaderCache->descriptorSets.lightsDescriptorSets[activeImage],
                    shaderCache->descriptorSets.pbrDescriptorSet
                };
                geometryDrawParams.meshlet = &meshlet;
                vulkanDevice->DrawMeshlet(geometryDrawParams);
            }
        }

        shaderCache->renderpasses.geometryPass->End(commandBuffer);
    }

    void VulkanRenderer::DrawUIPass(const VkCommandBuffer commandBuffer)
    {
        vulkanDevice->SetViewportAndScissor(vulkanSwapChain->GetExtent(), commandBuffer);
        shaderCache->renderpasses.presentPass->Begin(commandBuffer, framebuffers.presentFramebuffers[activeImage],
                                                     vulkanSwapChain->GetExtent());

        DrawCommandParams screenRectDrawParams{};
        screenRectDrawParams.commandBuffer = commandBuffer;
        screenRectDrawParams.pipeline = shaderCache->pipelines.present->GetPipeline();
        screenRectDrawParams.pipelineLayout = shaderCache->pipelines.present->GetPipelineLayout();
        screenRectDrawParams.descriptorSets = {
            shaderCache->descriptorSets.presentDescriptorSets[activeImage],
        };
        screenRectDrawParams.meshlet = screenRect.get();

        vulkanDevice->DrawMeshlet(screenRectDrawParams);
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

        shaderCache->renderpasses.presentPass->End(commandBuffer);
    }

    void VulkanRenderer::IdleWait()
    {
        vkDeviceWaitIdle(vulkanDevice->GetDevice());
    }

    void VulkanRenderer::Resize(const int width, const int height)
    {
        Renderer::Resize(width, height);

        viewportWidth = width;
        viewportHeight = height;

        IdleWait();
        vulkanDevice->GetReadyToResize();
    }

    void VulkanRenderer::CreateSwapchain()
    {
        LOG_TRACE("Vulkan: create swapchain");
        vulkanSwapChain = nullptr;

        const auto swapChainSupport = VulkanUtils::QuerySwapChainSupport(vulkanDevice->GetPhysicalDevice(), vulkanDevice->GetSurface());
        const VkSurfaceFormatKHR surfaceFormat = VulkanUtils::ChooseSwapSurfaceFormat(swapChainSupport.formats);

        const uint32_t imageCount = VulkanUtils::GetSwapchainImageCount(vulkanDevice->GetPhysicalDevice(), vulkanDevice->GetSurface());
        vulkanSwapChain = VulkanSwapchain::Builder(vulkanDevice.get())
                .SetResolution(viewportWidth, viewportHeight)
                .SetPresentMode(VK_PRESENT_MODE_IMMEDIATE_KHR)
                .SetImageCount(imageCount)
                .SetImageFormat(surfaceFormat.format)
                .Build();
    }

    void VulkanRenderer::CreateFramebuffers()
    {
        const uint32_t imageCount = VulkanUtils::GetSwapchainImageCount(vulkanDevice->GetPhysicalDevice(), vulkanDevice->GetSurface());
        framebuffers.geometryFramebuffers.resize(imageCount);
        framebuffers.shadowMapFramebuffers.resize(imageCount);
        framebuffers.directionalShadowMaps.resize(imageCount);
        framebuffers.presentFramebuffers.resize(imageCount);

        for (size_t i = 0; i < imageCount; i++)
        {
            framebuffers.geometryFramebuffers[i] = VulkanFramebuffer::Builder(vulkanDevice.get())
                    .SetRenderpass(shaderCache->renderpasses.geometryPass)
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
                    .Build(vulkanDevice.get());

            framebuffers.shadowMapFramebuffers[i] = VulkanFramebuffer::Builder(vulkanDevice.get())
                    .SetRenderpass(shaderCache->renderpasses.shadowMapPass)
                    .SetResolution(4096, 4096)
                    .AddAttachment(framebuffers.directionalShadowMaps[i]->GetImageView())
                    .Build();

            framebuffers.presentFramebuffers[i] = VulkanFramebuffer::Builder(vulkanDevice.get())
                    .SetRenderpass(shaderCache->renderpasses.presentPass)
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
            vulkanDevice.get(),
            sizeof(TransformsBuffer),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU);

        VkDescriptorBufferInfo info{};
        info.buffer = descriptorBuffers.transformsBuffer->GetBuffer();
        info.offset = 0;
        info.range = sizeof(TransformsBuffer);

        VulkanDescriptorWriter(*shaderCache->descriptorSetLayouts.transformDescriptorSetLayout, vulkanDevice->GetShaderDescriptorPool())
                .WriteBuffer(0, &info)
                .Build(shaderCache->descriptorSets.transformDescriptorSet);
    }

    void VulkanRenderer::CreateLightsBuffer()
    {
        if (!descriptorBuffers.lightsBuffer)
        {
            descriptorBuffers.lightsBuffer = CreateRef<VulkanBuffer>(
                vulkanDevice.get(),
                sizeof(LightsBuffer),
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                VMA_MEMORY_USAGE_CPU_TO_GPU);

            VkDescriptorBufferInfo info{};
            info.buffer = descriptorBuffers.lightsBuffer->GetBuffer();
            info.offset = 0;
            info.range = sizeof(LightsBuffer);

            const uint32_t imageCount = VulkanUtils::GetSwapchainImageCount(vulkanDevice->GetPhysicalDevice(), vulkanDevice->GetSurface());
            shaderCache->descriptorSets.lightsDescriptorSets.resize(imageCount);

            for (size_t i = 0; i < imageCount; i++)
            {
                VkDescriptorImageInfo shadowMapInfo{};
                shadowMapInfo.sampler = framebuffers.directionalShadowMaps[i]->GetSampler();
                shadowMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                shadowMapInfo.imageView = framebuffers.directionalShadowMaps[i]->GetImageView();

                VulkanDescriptorWriter(*shaderCache->descriptorSetLayouts.lightsDescriptorSetLayout,
                                       vulkanDevice->GetShaderDescriptorPool())
                        .WriteBuffer(0, &info)
                        .WriteImage(1, &shadowMapInfo)
                        .Build(shaderCache->descriptorSets.lightsDescriptorSets[i]);
            }
        } else
        {
            const uint32_t imageCount = VulkanUtils::GetSwapchainImageCount(vulkanDevice->GetPhysicalDevice(), vulkanDevice->GetSurface());
            shaderCache->descriptorSets.lightsDescriptorSets.resize(imageCount);

            for (size_t i = 0; i < imageCount; i++)
            {
                VkDescriptorImageInfo shadowMapInfo{};
                shadowMapInfo.sampler = framebuffers.directionalShadowMaps[i]->GetSampler();
                shadowMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                shadowMapInfo.imageView = framebuffers.directionalShadowMaps[i]->GetImageView();

                VulkanDescriptorWriter(*shaderCache->descriptorSetLayouts.lightsDescriptorSetLayout,
                                       vulkanDevice->GetShaderDescriptorPool())
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

    void VulkanRenderer::UploadCubemapTexture(Ref<VulkanCubeMapTexture> cubemap)
    {
        VulkanDescriptorSetLayout& skyboxDescriptorSetLayoutLayout = *shaderCache->descriptorSetLayouts.cubemapDescriptorSetLayout;
        VulkanDescriptorWriter writer = VulkanDescriptorWriter(skyboxDescriptorSetLayoutLayout, vulkanDevice->GetShaderDescriptorPool());

        VkDescriptorImageInfo info{};
        info.sampler = cubemap->GetSampler();
        info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        info.imageView = cubemap->GetImageView();

        writer.WriteImage(0, &info);

        writer.Build(shaderCache->descriptorSets.cubemapDescriptorSet);
    }

    void VulkanRenderer::PreparePresentPass()
    {
        const uint32_t imageCount = VulkanUtils::GetSwapchainImageCount(vulkanDevice->GetPhysicalDevice(), vulkanDevice->GetSurface());
        shaderCache->descriptorSets.presentDescriptorSets.clear();
        shaderCache->descriptorSets.presentDescriptorSets.resize(imageCount);

        for (size_t i = 0; i < imageCount; i++)
        {
            VkDescriptorImageInfo renderImageInfo{};
            renderImageInfo.sampler = framebuffers.geometryFramebuffers[i]->GetAttachments()[0].sampler;
            renderImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            renderImageInfo.imageView = framebuffers.geometryFramebuffers[i]->GetAttachments()[0].imageView;

            auto writer = VulkanDescriptorWriter(*shaderCache->descriptorSetLayouts.presentDescriptorSetLayout,
                                                 vulkanDevice->GetShaderDescriptorPool())
                    .WriteImage(0, &renderImageInfo);

            if (!shaderCache->descriptorSets.presentDescriptorSets[i])
                writer.Build(shaderCache->descriptorSets.presentDescriptorSets[i]);
            else
                writer.Overwrite(shaderCache->descriptorSets.presentDescriptorSets[i]);
        }
    }

    void VulkanRenderer::PreparePBR()
    {
        VkDescriptorImageInfo irradianceMapInfo{};
        irradianceMapInfo.sampler = irradianceMap->GetSampler();
        irradianceMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        irradianceMapInfo.imageView = irradianceMap->GetImageView();

        VkDescriptorImageInfo prefilterMapInfo{};
        prefilterMapInfo.sampler = prefilterMap->GetSampler();
        prefilterMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        prefilterMapInfo.imageView = prefilterMap->GetImageView();

        VkDescriptorImageInfo brdfLUTInfo{};
        brdfLUTInfo.sampler = framebuffers.iblBRDFFramebuffer->attachments[0].sampler;
        brdfLUTInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        brdfLUTInfo.imageView = framebuffers.iblBRDFFramebuffer->attachments[0].imageView;

        VulkanDescriptorWriter(*shaderCache->descriptorSetLayouts.pbrDescriptorSetLayout, vulkanDevice->GetShaderDescriptorPool())
                .WriteImage(0, &irradianceMapInfo)
                .WriteImage(1, &prefilterMapInfo)
                .WriteImage(2, &brdfLUTInfo)
                .Build(shaderCache->descriptorSets.pbrDescriptorSet);
    }
}
