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
    Pipelines VulkanRenderer::pipelines;
    Renderpass VulkanRenderer::renderpasses;

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

        CreateTransformsBuffer();

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
                ImageFormat::RGBA8_UNORM,
                ImageFormat::RGBA8_UNORM,
                ImageFormat::RGBA8_UNORM,
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

        cubeMesh = ResourceManager::LoadMesh(vulkanDevice.get(), "resources/models/cube.obj");

        LOG_TRACE("Load skybox");
        cubemapTexture = ResourceManager::LoadHDRCubeMap(vulkanDevice.get(), "resources/environment/kloppenheim_06_puresky_4k.hdr");
        //cubemapTexture = ResourceManager::LoadHDRCubeMap(vulkanDevice.get(), "resources/environment/newport_loft.hdr");
        //cubemapTexture = ResourceManager::LoadHDRCubeMap(vulkanDevice.get(), "resources/environment/champagne_castle_1_4k.hdr");

        PrepareSkyboxPass();

        screenRect = CreateScope<VulkanMeshlet>(vulkanDevice.get(), Primitives::RECTANGLE_VERTICES, Primitives::RECTANGLE_INDICES);

        LOG_TRACE("Load scene");
        //completeScene = ResourceManager::LoadScene(vulkanDevice.get(), "resources/sponza/Sponza.gltf");
        //completeScene = ResourceManager::LoadScene(vulkanDevice.get(), "resources/vertex_color/vertex_color.gltf");
        //completeScene = ResourceManager::LoadScene(vulkanDevice.get(), "resources/normal_tangent/NormalTangentTest.gltf");
        //completeScene = ResourceManager::LoadScene(vulkanDevice.get(), "resources/gltf/multiple_spheres.gltf");
        completeScene = ResourceManager::LoadScene(vulkanDevice.get(), "resources/chess/ABeautifulGame.gltf");

        transform.m_Position = glm::vec3(0.0f, 0.0f, -1.0f);
    }

    void VulkanRenderer::DrawGeometryPass(const Ref<Camera>& camera, const VkCommandBuffer commandBuffer) const
    {
        renderpasses.geometryPass->Begin(commandBuffer, geometryFramebuffers[activeImage], vulkanSwapChain->GetExtent());


        DrawCommandParams skyboxDrawParams{};
        skyboxDrawParams.commandBuffer = commandBuffer;
        skyboxDrawParams.meshlet = &cubeMesh->GetMeshlets()[0];
        skyboxDrawParams.pipeline = pipelines.skyBox->GetPipeline();
        skyboxDrawParams.pipelineLayout = pipelines.skyBox->GetPipelineLayout();
        skyboxDrawParams.descriptorSets = {skyboxDescriptorSet, transformDescriptorSet};

        vulkanDevice->DrawMeshlet(skyboxDrawParams);


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
                    transformDescriptorSet,
                    skyboxDescriptorSet
                };
                geometryDrawParams.meshlet = &meshlet;
                vulkanDevice->DrawMeshlet(geometryDrawParams);
            }
        }

        renderpasses.geometryPass->End(commandBuffer);
    }


    void VulkanRenderer::DrawFrame(float deltaTime, Ref<Camera> camera)
    {
        UpdateTransformsBuffer(camera);
        vulkanDevice->DrawFrame(vulkanSwapChain->GetSwapChain(), vulkanSwapChain->GetExtent(),
                                [&](const VkCommandBuffer commandBuffer, uint32_t, const uint32_t imageIndex) {
                                    activeImage = imageIndex;

                                    DrawGeometryPass(camera, commandBuffer);
                                    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
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
        presentFramebuffers.resize(imageCount);

        for (size_t i = 0; i < imageCount; i++)
        {
            geometryFramebuffers[i] = VulkanFramebuffer::Builder(vulkanDevice.get())
                    .SetRenderpass(renderpasses.geometryPass)
                    .SetResolution(viewportWidth, viewportHeight)
                    .AddAttachment(vulkanSwapChain->GetImageViews()[i])
                    .AddAttachment(ImageFormat::RGBA8_UNORM)
                    .AddAttachment(ImageFormat::RGBA8_UNORM)
                    .AddAttachment(ImageFormat::RGBA8_UNORM)
                    .AddAttachment(ImageFormat::RGBA8_UNORM)
                    .AddAttachment(ImageFormat::DEPTH24_STENCIL8)
                    .Build();
        }
    }

    void VulkanRenderer::CreateRenderPasses() const
    {
        LOG_TRACE("Vulkan: create renderpass");
        renderpasses.skyboxPass = VulkanRenderPass::Builder(vulkanDevice.get())
                .AddColorAttachment(VK_FORMAT_R8G8B8A8_UNORM)
                .AddDepthAttachment()
                .Build();

        renderpasses.geometryPass = VulkanRenderPass::Builder(vulkanDevice.get())
                .AddColorAttachment(VK_FORMAT_R8G8B8A8_UNORM)
                .AddColorAttachment(VK_FORMAT_R8G8B8A8_UNORM)
                .AddColorAttachment(VK_FORMAT_R8G8B8A8_UNORM)
                .AddColorAttachment(VK_FORMAT_R8G8B8A8_UNORM)
                .AddColorAttachment(VK_FORMAT_R8G8B8A8_UNORM)
                .AddDepthAttachment()
                .Build();
    }

    void VulkanRenderer::ResizeSwapchain()
    {
        IdleWait();
        for (size_t i = 0; i < cubeMapFramebuffers.size(); i++)
            cubeMapFramebuffers[i] = nullptr;

        geometryFramebuffers.clear();
        presentFramebuffers.clear();

        CreateSwapchain();
        CreateFramebuffers();
    }

    void VulkanRenderer::CreateTransformsBuffer()
    {
        transformsBuffer = CreateRef<VulkanBuffer>(
            vulkanDevice.get(),
            sizeof(TransformsBuffer),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU);

        VkDescriptorBufferInfo info{};
        info.buffer = transformsBuffer->GetBuffer();
        info.offset = 0;
        info.range = sizeof(TransformsBuffer);

        VulkanDescriptorWriter(*descriptorSetLayouts.transformDescriptorSetLayout, vulkanDevice->GetShaderDescriptorPool())
                .WriteBuffer(0, &info)
                .Build(transformDescriptorSet);
    }

    void VulkanRenderer::UpdateTransformsBuffer(const Ref<Camera>& camera) const
    {
        TransformsBuffer bufferData;
        bufferData.cameraPosition = glm::vec4(camera->GetTransform().m_Position, 1.0f);
        bufferData.view = camera->GetView();
        bufferData.proj = camera->GetProjection();

        memcpy(transformsBuffer->GetMappedData(), &bufferData, sizeof(TransformsBuffer));
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

        writer.Build(skyboxDescriptorSet);
    }
}
