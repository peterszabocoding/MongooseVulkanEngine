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
    DescriptorSetLayouts VulkanRenderer::descriptorSetLayouts;
    DescriptorBuffers VulkanRenderer::descriptorBuffers;
    DescriptorSets VulkanRenderer::descriptorSets;
    Renderpass VulkanRenderer::renderpasses;
    Pipelines VulkanRenderer::pipelines;

    VulkanRenderer::~VulkanRenderer() {}

    void VulkanRenderer::Init(const int width, const int height)
    {
        viewportWidth = width;
        viewportHeight = height;

        LOG_TRACE("VulkanRenderer::Init()");
        vulkanDevice = CreateScope<VulkanDevice>(width, height, glfwWindow);

        CreateSwapchain();
        CreateRenderPasses();
        CreateFramebuffers();

        descriptorSetLayouts.transformDescriptorSetLayout = VulkanDescriptorSetLayout::Builder(vulkanDevice.get())
                .AddBinding({
                    0, DescriptorSetBindingType::UniformBuffer,
                    {
                        DescriptorSetShaderStage::VertexShader,
                        DescriptorSetShaderStage::FragmentShader
                    }
                })
                .Build();

        descriptorSetLayouts.materialDescriptorSetLayout = VulkanDescriptorSetLayout::Builder(vulkanDevice.get())
                .AddBinding({0, DescriptorSetBindingType::UniformBuffer, {DescriptorSetShaderStage::FragmentShader}})
                .AddBinding({1, DescriptorSetBindingType::TextureSampler, {DescriptorSetShaderStage::FragmentShader}})
                .AddBinding({2, DescriptorSetBindingType::TextureSampler, {DescriptorSetShaderStage::FragmentShader}})
                .AddBinding({3, DescriptorSetBindingType::TextureSampler, {DescriptorSetShaderStage::FragmentShader}})
                .Build();

        descriptorSetLayouts.skyboxDescriptorSetLayout = VulkanDescriptorSetLayout::Builder(vulkanDevice.get())
                .AddBinding({0, DescriptorSetBindingType::TextureSampler, {DescriptorSetShaderStage::FragmentShader}})
                .Build();

        descriptorSetLayouts.lightsDescriptorSetLayout = VulkanDescriptorSetLayout::Builder(vulkanDevice.get())
                .AddBinding({
                    0, DescriptorSetBindingType::UniformBuffer,
                    {
                        DescriptorSetShaderStage::VertexShader,
                        DescriptorSetShaderStage::FragmentShader
                    }
                })
                .AddBinding({1, DescriptorSetBindingType::TextureSampler, {DescriptorSetShaderStage::FragmentShader}})
                .Build();

        descriptorSetLayouts.presentDescriptorSetLayout = VulkanDescriptorSetLayout::Builder(vulkanDevice.get())
                        .AddBinding({0, DescriptorSetBindingType::TextureSampler, {DescriptorSetShaderStage::FragmentShader}})
                        .Build();

        CreateTransformsBuffer();
        CreateLightsBuffer();

        LOG_TRACE("Build pipelines");

        LOG_TRACE("Building skybox pipeline");
        PipelineConfig skyboxPipelineConfig; {
            skyboxPipelineConfig.vertexShaderPath = "shader/spv/skybox.vert.spv";
            skyboxPipelineConfig.fragmentShaderPath = "shader/spv/skybox.frag.spv";

            skyboxPipelineConfig.cullMode = PipelineCullMode::Front;
            skyboxPipelineConfig.polygonMode = PipelinePolygonMode::Fill;
            skyboxPipelineConfig.frontFace = PipelineFrontFace::Counter_clockwise;

            skyboxPipelineConfig.descriptorSetLayouts = {
                descriptorSetLayouts.skyboxDescriptorSetLayout,
                descriptorSetLayouts.transformDescriptorSetLayout,
            };

            skyboxPipelineConfig.colorAttachments = {
                ImageFormat::RGBA8_UNORM,
            };

            skyboxPipelineConfig.disableBlending = true;
            skyboxPipelineConfig.enableDepthTest = false;
            skyboxPipelineConfig.depthAttachment = ImageFormat::DEPTH24_STENCIL8;

            skyboxPipelineConfig.renderPass = renderpasses.geometryPass;
        }
        pipelines.skyBox = VulkanPipeline::Builder().Build(vulkanDevice.get(), skyboxPipelineConfig);

        LOG_TRACE("Building geometry pipeline");
        PipelineConfig geometryPipelineConfig; {
            geometryPipelineConfig.vertexShaderPath = "shader/spv/gbuffer.vert.spv";
            geometryPipelineConfig.fragmentShaderPath = "shader/spv/gbuffer.frag.spv";

            geometryPipelineConfig.cullMode = PipelineCullMode::Back;
            geometryPipelineConfig.polygonMode = PipelinePolygonMode::Fill;
            geometryPipelineConfig.frontFace = PipelineFrontFace::Counter_clockwise;

            geometryPipelineConfig.descriptorSetLayouts = {
                descriptorSetLayouts.materialDescriptorSetLayout,
                descriptorSetLayouts.transformDescriptorSetLayout,
                descriptorSetLayouts.skyboxDescriptorSetLayout,
                descriptorSetLayouts.lightsDescriptorSetLayout,
            };

            geometryPipelineConfig.colorAttachments = {
                ImageFormat::RGBA8_UNORM,
                ImageFormat::RGBA8_UNORM,
                ImageFormat::RGBA8_UNORM,
                ImageFormat::RGBA8_UNORM,
                ImageFormat::RGBA8_UNORM,
            };

            geometryPipelineConfig.disableBlending = true;
            geometryPipelineConfig.enableDepthTest = true;
            geometryPipelineConfig.depthAttachment = ImageFormat::DEPTH24_STENCIL8;

            geometryPipelineConfig.renderPass = renderpasses.geometryPass;

            geometryPipelineConfig.pushConstantData.shaderStageBits = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            geometryPipelineConfig.pushConstantData.offset = 0;
            geometryPipelineConfig.pushConstantData.size = sizeof(SimplePushConstantData);
        }
        pipelines.geometry = VulkanPipeline::Builder().Build(vulkanDevice.get(), geometryPipelineConfig);

        LOG_TRACE("Building directional shadow map pipeline");
        PipelineConfig dirShadowMapPipelineConfig; {
            dirShadowMapPipelineConfig.vertexShaderPath = "shader/spv/shadow_map.vert.spv";
            dirShadowMapPipelineConfig.fragmentShaderPath = "shader/spv/empty.frag.spv";

            dirShadowMapPipelineConfig.cullMode = PipelineCullMode::Front;
            dirShadowMapPipelineConfig.polygonMode = PipelinePolygonMode::Fill;
            dirShadowMapPipelineConfig.frontFace = PipelineFrontFace::Counter_clockwise;

            dirShadowMapPipelineConfig.descriptorSetLayouts = {
                descriptorSetLayouts.lightsDescriptorSetLayout,
            };

            dirShadowMapPipelineConfig.disableBlending = true;
            dirShadowMapPipelineConfig.enableDepthTest = true;
            dirShadowMapPipelineConfig.depthAttachment = ImageFormat::DEPTH32;

            dirShadowMapPipelineConfig.renderPass = renderpasses.shadowMapPass;

            dirShadowMapPipelineConfig.pushConstantData.shaderStageBits = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            dirShadowMapPipelineConfig.pushConstantData.offset = 0;
            dirShadowMapPipelineConfig.pushConstantData.size = sizeof(SimplePushConstantData);
        }
        pipelines.directionalShadowMap = VulkanPipeline::Builder().Build(vulkanDevice.get(), dirShadowMapPipelineConfig);

        LOG_TRACE("Building present pipeline");
        PipelineConfig presentPipelineConfig; {
            presentPipelineConfig.vertexShaderPath = "shader/spv/quad.vert.spv";
            presentPipelineConfig.fragmentShaderPath = "shader/spv/quad.frag.spv";

            presentPipelineConfig.cullMode = PipelineCullMode::Front;
            presentPipelineConfig.polygonMode = PipelinePolygonMode::Fill;
            presentPipelineConfig.frontFace = PipelineFrontFace::Counter_clockwise;

            presentPipelineConfig.descriptorSetLayouts = {
                descriptorSetLayouts.presentDescriptorSetLayout
            };

            presentPipelineConfig.colorAttachments = {
                ImageFormat::RGBA8_UNORM,
            };

            presentPipelineConfig.disableBlending = true;
            presentPipelineConfig.enableDepthTest = false;

            presentPipelineConfig.renderPass = renderpasses.presentPass;
        }
        pipelines.present = VulkanPipeline::Builder().Build(vulkanDevice.get(), presentPipelineConfig);

        cubeMesh = ResourceManager::LoadMesh(vulkanDevice.get(), "resources/models/cube.obj");

        LOG_TRACE("Load skybox");
        cubemapTexture = ResourceManager::LoadHDRCubeMap(vulkanDevice.get(), "resources/environment/newport_loft.hdr");
        //cubemapTexture = ResourceManager::LoadHDRCubeMap(vulkanDevice.get(), "resources/environment/champagne_castle_1_4k.hdr");

        PrepareSkyboxPass();
        PreparePresentPass();

        screenRect = CreateScope<VulkanMeshlet>(vulkanDevice.get(), Primitives::RECTANGLE_VERTICES, Primitives::RECTANGLE_INDICES);

        LOG_TRACE("Load scene");
        //completeScene = ResourceManager::LoadScene(vulkanDevice.get(), "resources/sponza/Sponza.gltf");
        //completeScene = ResourceManager::LoadScene(vulkanDevice.get(), "resources/vertex_color/vertex_color.gltf");
        //completeScene = ResourceManager::LoadScene(vulkanDevice.get(), "resources/normal_tangent/NormalTangentTest.gltf");
        //completeScene = ResourceManager::LoadScene(vulkanDevice.get(), "resources/gltf/multiple_spheres.gltf");
        //completeScene = ResourceManager::LoadScene(vulkanDevice.get(), "resources/chess/ABeautifulGame.gltf");
        completeScene = ResourceManager::LoadScene(vulkanDevice.get(), "resources/tests/orientation_test/orientation_test.gltf");

        directionalLight.direction = normalize(glm::vec3(0.0f, -2.0f, -1.0f));
    }

    void VulkanRenderer::DrawSkybox(const VkCommandBuffer commandBuffer) const
    {
        DrawCommandParams skyboxDrawParams{};
        skyboxDrawParams.commandBuffer = commandBuffer;
        skyboxDrawParams.meshlet = &cubeMesh->GetMeshlets()[0];
        skyboxDrawParams.pipeline = pipelines.skyBox->GetPipeline();
        skyboxDrawParams.pipelineLayout = pipelines.skyBox->GetPipelineLayout();
        skyboxDrawParams.descriptorSets = {descriptorSets.skyboxDescriptorSet, descriptorSets.transformDescriptorSet};

        vulkanDevice->DrawMeshlet(skyboxDrawParams);
    }

    void VulkanRenderer::DrawDirectionalShadowMapPass(const VkCommandBuffer commandBuffer)
    {
        VkExtent2D renderExtent = vulkanSwapChain->GetExtent();
        VkExtent2D extent = {2048, 2048};
        //renderpasses.shadowMapPass->Begin(commandBuffer, shadowMapFramebuffers[activeImage], renderExtent);

        vulkanDevice->SetViewportAndScissor(extent, commandBuffer);
        renderpasses.shadowMapPass->Begin(commandBuffer, shadowMapFramebuffers[activeImage], extent);

        DrawCommandParams geometryDrawParams{};
        geometryDrawParams.commandBuffer = commandBuffer;
        geometryDrawParams.pipeline = pipelines.directionalShadowMap->GetPipeline();
        geometryDrawParams.pipelineLayout = pipelines.directionalShadowMap->GetPipelineLayout();

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
                    descriptorSets.lightsDescriptorSets[activeImage]
                };
                geometryDrawParams.meshlet = &meshlet;
                vulkanDevice->DrawMeshlet(geometryDrawParams);
            }
        }

        renderpasses.shadowMapPass->End(commandBuffer);
    }

    void VulkanRenderer::DrawGeometryPass(const Ref<Camera>& camera, const VkCommandBuffer commandBuffer) const
    {
        vulkanDevice->SetViewportAndScissor(vulkanSwapChain->GetExtent(), commandBuffer);
        renderpasses.geometryPass->Begin(commandBuffer, geometryFramebuffers[activeImage], vulkanSwapChain->GetExtent());

        DrawSkybox(commandBuffer);

        DrawCommandParams geometryDrawParams{};
        geometryDrawParams.commandBuffer = commandBuffer;
        geometryDrawParams.pipeline = pipelines.geometry->GetPipeline();
        geometryDrawParams.pipelineLayout = pipelines.geometry->GetPipelineLayout();

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
                    descriptorSets.transformDescriptorSet,
                    descriptorSets.skyboxDescriptorSet,
                    descriptorSets.lightsDescriptorSets[activeImage]
                };
                geometryDrawParams.meshlet = &meshlet;
                vulkanDevice->DrawMeshlet(geometryDrawParams);
            }
        }

        renderpasses.geometryPass->End(commandBuffer);
    }

    void VulkanRenderer::DrawUIPass(const VkCommandBuffer commandBuffer)
    {
        vulkanDevice->SetViewportAndScissor(vulkanSwapChain->GetExtent(), commandBuffer);
        renderpasses.presentPass->Begin(commandBuffer, presentFramebuffers[activeImage], vulkanSwapChain->GetExtent());

        DrawCommandParams screenRectDrawParams{};
        screenRectDrawParams.commandBuffer = commandBuffer;
        screenRectDrawParams.pipeline = pipelines.present->GetPipeline();
        screenRectDrawParams.pipelineLayout = pipelines.present->GetPipelineLayout();
        screenRectDrawParams.descriptorSets = {
            descriptorSets.presentDescriptorSets[activeImage],
        };
        screenRectDrawParams.meshlet = screenRect.get();

        vulkanDevice->DrawMeshlet(screenRectDrawParams);
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

        renderpasses.presentPass->End(commandBuffer);
    }

    void VulkanRenderer::DrawFrame(float deltaTime, Ref<Camera> camera)
    {
        UpdateTransformsBuffer(camera);
        UpdateLightsBuffer(deltaTime);

        vulkanDevice->DrawFrame(vulkanSwapChain->GetSwapChain(), vulkanSwapChain->GetExtent(),
                                [&](const VkCommandBuffer commandBuffer, uint32_t, const uint32_t imageIndex) {
                                    activeImage = imageIndex;

                                    directionalShadowMaps[activeImage]->PrepareToDepthRendering();
                                    DrawDirectionalShadowMapPass(commandBuffer);
                                    directionalShadowMaps[activeImage]->PrepareToShadowRendering();

                                    DrawGeometryPass(camera, commandBuffer);
                                    DrawUIPass(commandBuffer);
                                }, std::bind(&VulkanRenderer::ResizeSwapchain, this));
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
        geometryFramebuffers.resize(imageCount);
        shadowMapFramebuffers.resize(imageCount);
        directionalShadowMaps.resize(imageCount);
        presentFramebuffers.resize(imageCount);

        for (size_t i = 0; i < imageCount; i++)
        {
            geometryFramebuffers[i] = VulkanFramebuffer::Builder(vulkanDevice.get())
                    .SetRenderpass(renderpasses.geometryPass)
                    .SetResolution(viewportWidth, viewportHeight)
                    .AddAttachment(ImageFormat::RGBA8_UNORM)
                    .AddAttachment(ImageFormat::RGBA8_UNORM)
                    .AddAttachment(ImageFormat::RGBA8_UNORM)
                    .AddAttachment(ImageFormat::RGBA8_UNORM)
                    .AddAttachment(ImageFormat::RGBA8_UNORM)
                    .AddAttachment(ImageFormat::DEPTH24_STENCIL8)
                    .Build();

            directionalShadowMaps[i] = VulkanShadowMap::Builder()
                    .SetResolution(2048, 2048)
                    .Build(vulkanDevice.get());

            shadowMapFramebuffers[i] = VulkanFramebuffer::Builder(vulkanDevice.get())
                    .SetRenderpass(renderpasses.shadowMapPass)
                    .SetResolution(2048, 2048)
                    .AddAttachment(directionalShadowMaps[i]->GetImageView())
                    .Build();

            presentFramebuffers[i] = VulkanFramebuffer::Builder(vulkanDevice.get())
                    .SetRenderpass(renderpasses.presentPass)
                    .SetResolution(viewportWidth, viewportHeight)
                    .AddAttachment(vulkanSwapChain->GetImageViews()[i])
                    .Build();
        }
    }

    void VulkanRenderer::CreateRenderPasses() const
    {
        LOG_TRACE("Vulkan: create renderpass");
        renderpasses.skyboxPass = VulkanRenderPass::Builder(vulkanDevice.get())
                .AddColorAttachment(VK_FORMAT_R8G8B8A8_UNORM, false)
                .AddDepthAttachment()
                .Build();

        renderpasses.geometryPass = VulkanRenderPass::Builder(vulkanDevice.get())
                .AddColorAttachment(VK_FORMAT_R8G8B8A8_UNORM, false)
                .AddColorAttachment(VK_FORMAT_R8G8B8A8_UNORM, false)
                .AddColorAttachment(VK_FORMAT_R8G8B8A8_UNORM, false)
                .AddColorAttachment(VK_FORMAT_R8G8B8A8_UNORM, false)
                .AddColorAttachment(VK_FORMAT_R8G8B8A8_UNORM, false)
                .AddDepthAttachment()
                .Build();

        renderpasses.shadowMapPass = VulkanRenderPass::Builder(vulkanDevice.get())
                .AddDepthAttachment(VK_FORMAT_D32_SFLOAT)
                .Build();

        renderpasses.presentPass = VulkanRenderPass::Builder(vulkanDevice.get())
                .AddColorAttachment(VK_FORMAT_R8G8B8A8_UNORM, true)
                .Build();
    }

    void VulkanRenderer::ResizeSwapchain()
    {
        IdleWait();

        geometryFramebuffers.clear();
        geometryFramebuffers.clear();
        geometryFramebuffers.clear();
        presentFramebuffers.clear();

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

        VulkanDescriptorWriter(*descriptorSetLayouts.transformDescriptorSetLayout, vulkanDevice->GetShaderDescriptorPool())
                .WriteBuffer(0, &info)
                .Build(descriptorSets.transformDescriptorSet);
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
            descriptorSets.lightsDescriptorSets.resize(imageCount);

            for (size_t i = 0; i < imageCount; i++)
            {
                VkDescriptorImageInfo shadowMapInfo{};
                shadowMapInfo.sampler = directionalShadowMaps[i]->GetSampler();
                shadowMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                shadowMapInfo.imageView = directionalShadowMaps[i]->GetImageView();

                VulkanDescriptorWriter(*descriptorSetLayouts.lightsDescriptorSetLayout, vulkanDevice->GetShaderDescriptorPool())
                        .WriteBuffer(0, &info)
                        .WriteImage(1, &shadowMapInfo)
                        .Build(descriptorSets.lightsDescriptorSets[i]);
            }
        } else
        {
            const uint32_t imageCount = VulkanUtils::GetSwapchainImageCount(vulkanDevice->GetPhysicalDevice(), vulkanDevice->GetSurface());
            descriptorSets.lightsDescriptorSets.resize(imageCount);

            for (size_t i = 0; i < imageCount; i++)
            {
                VkDescriptorImageInfo shadowMapInfo{};
                shadowMapInfo.sampler = directionalShadowMaps[i]->GetSampler();
                shadowMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                shadowMapInfo.imageView = directionalShadowMaps[i]->GetImageView();

                VulkanDescriptorWriter(*descriptorSetLayouts.lightsDescriptorSetLayout, vulkanDevice->GetShaderDescriptorPool())
                        .WriteImage(1, &shadowMapInfo)
                        .Overwrite(descriptorSets.lightsDescriptorSets[i]);
            }
        }
    }

    void VulkanRenderer::UpdateTransformsBuffer(const Ref<Camera>& camera) const
    {
        TransformsBuffer bufferData;
        bufferData.cameraPosition = glm::vec4(camera->GetTransform().m_Position, 1.0f);
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

    void VulkanRenderer::PrepareSkyboxPass()
    {
        VulkanDescriptorSetLayout& skyboxDescriptorSetLayoutLayout = *pipelines.skyBox->GetDescriptorSetLayouts()[0];
        VulkanDescriptorWriter writer = VulkanDescriptorWriter(skyboxDescriptorSetLayoutLayout, vulkanDevice->GetShaderDescriptorPool());

        VkDescriptorImageInfo info{};
        info.sampler = cubemapTexture->GetSampler();
        info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        info.imageView = cubemapTexture->GetImageView();

        writer.WriteImage(0, &info);

        writer.Build(descriptorSets.skyboxDescriptorSet);
    }

    void VulkanRenderer::PreparePresentPass()
    {
        const uint32_t imageCount = VulkanUtils::GetSwapchainImageCount(vulkanDevice->GetPhysicalDevice(), vulkanDevice->GetSurface());
        descriptorSets.presentDescriptorSets.clear();
        descriptorSets.presentDescriptorSets.resize(imageCount);

        for (size_t i = 0; i < imageCount; i++)
        {
            VkDescriptorImageInfo renderImageInfo{};
            renderImageInfo.sampler = geometryFramebuffers[i]->GetAttachments()[0].sampler;
            renderImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            renderImageInfo.imageView = geometryFramebuffers[i]->GetAttachments()[0].imageView;

            auto writer = VulkanDescriptorWriter(*descriptorSetLayouts.presentDescriptorSetLayout, vulkanDevice->GetShaderDescriptorPool())
                    .WriteImage(0, &renderImageInfo);

            if (!descriptorSets.presentDescriptorSets[i])
                writer.Build(descriptorSets.presentDescriptorSets[i]);
            else
                writer.Overwrite(descriptorSets.presentDescriptorSets[i]);
        }
    }
}
