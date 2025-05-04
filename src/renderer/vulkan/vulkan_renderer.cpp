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
    Renderpass VulkanRenderer::renderpass;

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
        CreateTransformsBuffer();

        descriptorSetLayouts.materialDescriptorSetLayout = VulkanDescriptorSetLayout::Builder(vulkanDevice.get())
                .AddBinding({0, DescriptorSetBindingType::UniformBuffer, {DescriptorSetShaderStage::FragmentShader}})
                .AddBinding({1, DescriptorSetBindingType::TextureSampler, {DescriptorSetShaderStage::FragmentShader}})
                .AddBinding({2, DescriptorSetBindingType::TextureSampler, {DescriptorSetShaderStage::FragmentShader}})
                .AddBinding({3, DescriptorSetBindingType::TextureSampler, {DescriptorSetShaderStage::FragmentShader}})
                .Build();

        descriptorSetLayouts.lightingDescriptorSetLayout = VulkanDescriptorSetLayout::Builder(vulkanDevice.get())
                .AddBinding({0, DescriptorSetBindingType::TextureSampler, {DescriptorSetShaderStage::FragmentShader}})
                .AddBinding({1, DescriptorSetBindingType::TextureSampler, {DescriptorSetShaderStage::FragmentShader}})
                .AddBinding({2, DescriptorSetBindingType::TextureSampler, {DescriptorSetShaderStage::FragmentShader}})
                .AddBinding({3, DescriptorSetBindingType::TextureSampler, {DescriptorSetShaderStage::FragmentShader}})
                .Build();

        descriptorSetLayouts.skyboxDescriptorSetLayout = VulkanDescriptorSetLayout::Builder(vulkanDevice.get())
                .AddBinding({0, DescriptorSetBindingType::UniformBuffer, {DescriptorSetShaderStage::VertexShader}})
                .AddBinding({1, DescriptorSetBindingType::TextureSampler, {DescriptorSetShaderStage::FragmentShader}})
                .Build();

        LOG_TRACE("Build pipelines");

        LOG_TRACE("Building skybox pipeline");
        PipelineConfig skyboxPipelineConfig; {
            skyboxPipelineConfig.vertexShaderPath = "shader/spv/skybox.vert.spv";
            skyboxPipelineConfig.fragmentShaderPath = "shader/spv/skybox.frag.spv";

            skyboxPipelineConfig.cullMode = PipelineCullMode::Front;
            skyboxPipelineConfig.polygonMode = PipelinePolygonMode::Fill;
            skyboxPipelineConfig.frontFace = PipelineFrontFace::Counter_clockwise;

            skyboxPipelineConfig.descriptorSetLayout = descriptorSetLayouts.skyboxDescriptorSetLayout;

            skyboxPipelineConfig.colorAttachments = {
                ImageFormat::RGBA8_UNORM,
                ImageFormat::RGBA8_UNORM,
                ImageFormat::RGBA8_UNORM,
            };

            skyboxPipelineConfig.disableBlending = true;
            skyboxPipelineConfig.enableDepthTest = false;
            skyboxPipelineConfig.depthAttachment = ImageFormat::DEPTH24_STENCIL8;

            skyboxPipelineConfig.renderPass = renderpass.gBufferPass;
        }
        pipelines.skyBox = VulkanPipeline::Builder().Build(vulkanDevice.get(), skyboxPipelineConfig);

        LOG_TRACE("Building geometry pipeline");
        PipelineConfig geometryPipelineConfig; {
            geometryPipelineConfig.vertexShaderPath = "shader/spv/gbuffer.vert.spv";
            geometryPipelineConfig.fragmentShaderPath = "shader/spv/gbuffer.frag.spv";

            geometryPipelineConfig.cullMode = PipelineCullMode::Back;
            geometryPipelineConfig.polygonMode = PipelinePolygonMode::Fill;
            geometryPipelineConfig.frontFace = PipelineFrontFace::Counter_clockwise;

            geometryPipelineConfig.descriptorSetLayout = descriptorSetLayouts.materialDescriptorSetLayout;

            geometryPipelineConfig.colorAttachments = {
                ImageFormat::RGBA8_UNORM,
                ImageFormat::RGBA8_UNORM,
                ImageFormat::RGBA8_UNORM,
            };

            geometryPipelineConfig.disableBlending = true;
            geometryPipelineConfig.enableDepthTest = true;
            geometryPipelineConfig.depthAttachment = ImageFormat::DEPTH24_STENCIL8;

            geometryPipelineConfig.renderPass = renderpass.gBufferPass;

            geometryPipelineConfig.pushConstantData.shaderStageBits = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            geometryPipelineConfig.pushConstantData.offset = 0;
            geometryPipelineConfig.pushConstantData.size = sizeof(SimplePushConstantData);
        }
        pipelines.gBuffer = VulkanPipeline::Builder().Build(vulkanDevice.get(), geometryPipelineConfig);


        LOG_TRACE("Building lighting pipeline");
        PipelineConfig lightingPipelineConfig; {
            lightingPipelineConfig.vertexShaderPath = "shader/spv/screen.vert.spv";
            lightingPipelineConfig.fragmentShaderPath = "shader/spv/screen.frag.spv";

            lightingPipelineConfig.cullMode = PipelineCullMode::Back;
            lightingPipelineConfig.polygonMode = PipelinePolygonMode::Fill;
            lightingPipelineConfig.frontFace = PipelineFrontFace::Clockwise;

            lightingPipelineConfig.descriptorSetLayout = descriptorSetLayouts.lightingDescriptorSetLayout;

            lightingPipelineConfig.colorAttachments = {
                ImageFormat::RGBA8_UNORM,
            };

            lightingPipelineConfig.disableBlending = true;
            lightingPipelineConfig.enableDepthTest = false;
            lightingPipelineConfig.renderPass = renderpass.lightingPass;
        }
        pipelines.lighting = VulkanPipeline::Builder().Build(vulkanDevice.get(), lightingPipelineConfig);

        cubeMesh = ResourceManager::LoadMesh(vulkanDevice.get(), "resources/models/cube.obj");

        LOG_TRACE("Load skybox");
        cubemapTexture = ResourceManager::LoadHDRCubeMap(vulkanDevice.get(), "resources/environment/kloppenheim_06_puresky_4k.hdr");
        //cubemapTexture = ResourceManager::LoadHDRCubeMap(vulkanDevice.get(), "resources/environment/newport_loft.hdr");
        //cubemapTexture = ResourceManager::LoadHDRCubeMap(vulkanDevice.get(), "resources/environment/champagne_castle_1_4k.hdr");

        PrepareSkyboxPass();
        PrepareLightingPass();

        screenRect = CreateScope<VulkanMeshlet>(vulkanDevice.get(), Primitives::RECTANGLE_VERTICES, Primitives::RECTANGLE_INDICES);

        LOG_TRACE("Load scene");
        completeScene = ResourceManager::LoadScene(vulkanDevice.get(), "resources/sponza/Sponza.gltf");
        //completeScene = ResourceManager::LoadScene(vulkanDevice.get(), "resources/vertex_color/vertex_color.gltf");
        //completeScene = ResourceManager::LoadScene(vulkanDevice.get(), "resources/normal_tangent/NormalTangentTest.gltf");
        //completeScene = ResourceManager::LoadScene(vulkanDevice.get(), "resources/gltf/multiple_spheres.gltf");
        //completeScene = ResourceManager::LoadScene(vulkanDevice.get(), "resources/chess/ABeautifulGame.gltf");

        transform.m_Position = glm::vec3(0.0f, 0.0f, -1.0f);
    }

    void VulkanRenderer::DrawGeometryPass(float deltaTime, const Ref<Camera>& camera, const VkCommandBuffer commandBuffer) const
    {
        renderpass.gBufferPass->Begin(commandBuffer, gbufferFramebuffers[activeImage], vulkanSwapChain->GetExtent());

        vulkanDevice->DrawMeshlet(commandBuffer,
                                          cubeMesh->GetMeshlets()[0],
                                          pipelines.skyBox->GetPipeline(),
                                          pipelines.skyBox->GetPipelineLayout(), skyboxDescriptorSet);

        for (size_t i = 0; i < completeScene.meshes.size(); i++)
        {
            vulkanDevice->DrawMesh(commandBuffer,
                                   camera,
                                   completeScene.transforms[i],
                                   completeScene.meshes[i],
                                   pipelines.gBuffer);
        }

        renderpass.gBufferPass->End(commandBuffer);
    }

    void VulkanRenderer::DrawLightingPass(VkCommandBuffer commandBuffer)
    {
        renderpass.lightingPass->Begin(commandBuffer, presentFramebuffers[activeImage],
                                       vulkanSwapChain->GetExtent());

        vulkanDevice->DrawMeshlet(commandBuffer, *screenRect,
                                  pipelines.lighting->GetPipeline(),
                                  pipelines.lighting->GetPipelineLayout(), presentDescriptorSets[activeImage]);

        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

        renderpass.lightingPass->End(commandBuffer);
    }

    void VulkanRenderer::DrawFrame(float deltaTime, Ref<Camera> camera)
    {
        UpdateTransformsBuffer(camera);
        vulkanDevice->DrawFrame(vulkanSwapChain->GetSwapChain(), vulkanSwapChain->GetExtent(),
                                [&](VkCommandBuffer commandBuffer, uint32_t frameIndex, uint32_t imageIndex) {
                                    activeImage = imageIndex;

                                    DrawGeometryPass(deltaTime, camera, commandBuffer);
                                    DrawLightingPass(commandBuffer);
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
        gbufferFramebuffers.resize(imageCount);
        presentFramebuffers.resize(imageCount);

        for (size_t i = 0; i < imageCount; i++)
        {
            gbufferFramebuffers[i] = VulkanFramebuffer::Builder(vulkanDevice.get())
                    .SetRenderpass(renderpass.gBufferPass)
                    .SetResolution(viewportWidth, viewportHeight)
                    .AddAttachment(ImageFormat::RGBA8_UNORM)
                    .AddAttachment(ImageFormat::RGBA8_UNORM)
                    .AddAttachment(ImageFormat::RGBA8_UNORM)
                    .AddAttachment(ImageFormat::DEPTH24_STENCIL8)
                    .Build();
        }

        for (size_t i = 0; i < imageCount; i++)
        {
            presentFramebuffers[i] = VulkanFramebuffer::Builder(vulkanDevice.get())
                    .SetRenderpass(renderpass.lightingPass)
                    .SetResolution(viewportWidth, viewportHeight)
                    .AddAttachment(vulkanSwapChain->GetImageViews()[i])
                    .Build();
        }
    }

    void VulkanRenderer::CreateRenderPasses()
    {
        LOG_TRACE("Vulkan: create renderpass");
        renderpass.cubeMapPass = VulkanRenderPass::Builder(vulkanDevice.get())
                .AddColorAttachment(VK_FORMAT_R16G16B16A16_SFLOAT)
                .Build();

        renderpass.skyboxPass = VulkanRenderPass::Builder(vulkanDevice.get())
                .AddColorAttachment(VK_FORMAT_R8G8B8A8_UNORM)
                .AddDepthAttachment()
                .Build();

        renderpass.gBufferPass = VulkanRenderPass::Builder(vulkanDevice.get())
                .AddColorAttachment(VK_FORMAT_R8G8B8A8_UNORM)
                .AddColorAttachment(VK_FORMAT_R8G8B8A8_UNORM)
                .AddColorAttachment(VK_FORMAT_R8G8B8A8_UNORM)
                .AddDepthAttachment()
                .Build();

        renderpass.lightingPass = VulkanRenderPass::Builder(vulkanDevice.get())
                .AddColorAttachment(VK_FORMAT_R8G8B8A8_UNORM, glm::vec4(1.0))
                .Build();
    }

    void VulkanRenderer::ResizeSwapchain()
    {
        IdleWait();
        for (size_t i = 0; i < cubeMapFramebuffers.size(); i++)
            cubeMapFramebuffers[i] = nullptr;

        gbufferFramebuffers.clear();
        presentFramebuffers.clear();

        CreateSwapchain();
        CreateFramebuffers();
        PrepareLightingPass();
    }

    void VulkanRenderer::CreateTransformsBuffer()
    {
        transformsBuffer = CreateRef<VulkanBuffer>(
            vulkanDevice.get(),
            sizeof(TransformsBuffer),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU);
    }

    void VulkanRenderer::UpdateTransformsBuffer(const Ref<Camera>& camera)
    {
        TransformsBuffer bufferData;
        bufferData.view = camera->GetView();
        bufferData.proj = camera->GetProjection();

        memcpy(transformsBuffer->GetMappedData(), &bufferData, sizeof(TransformsBuffer));
    }

    void VulkanRenderer::PrepareLightingPass()
    {
        presentSampler = ImageSamplerBuilder(vulkanDevice.get()).Build();

        VulkanDescriptorWriter writer = VulkanDescriptorWriter(*pipelines.lighting->GetDescriptorSetLayout(),
                                                               vulkanDevice->GetShaderDescriptorPool());

        presentDescriptorSets.resize(gbufferFramebuffers.size());
        for (size_t i = 0; i < gbufferFramebuffers.size(); i++)
        {
            VkDescriptorImageInfo info{};
            info.sampler = presentSampler;
            info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            info.imageView = gbufferFramebuffers[i]->GetAttachments()[0].imageView;

            writer.WriteImage(0, &info);

            VkDescriptorImageInfo info2{};
            info2.sampler = presentSampler;
            info2.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            info2.imageView = gbufferFramebuffers[i]->GetAttachments()[1].imageView;

            writer.WriteImage(1, &info2);

            VkDescriptorImageInfo info3{};
            info3.sampler = presentSampler;
            info3.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            info3.imageView = gbufferFramebuffers[i]->GetAttachments()[2].imageView;

            writer.WriteImage(2, &info3);

            VkDescriptorImageInfo info4{};
            info4.sampler = cubemapTexture->GetSampler();
            info4.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            info4.imageView = cubemapTexture->GetImageView();

            writer.WriteImage(3, &info4);

            writer.Build(presentDescriptorSets[i]);
        }
    }

    void VulkanRenderer::PrepareSkyboxPass()
    {
        VulkanDescriptorWriter writer = VulkanDescriptorWriter(*pipelines.skyBox->GetDescriptorSetLayout(),
                                                               vulkanDevice->GetShaderDescriptorPool());


        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = transformsBuffer->GetBuffer();
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(TransformsBuffer);

        VkDescriptorImageInfo info{};
        info.sampler = cubemapTexture->GetSampler();
        info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        info.imageView = cubemapTexture->GetImageView();

        writer.WriteBuffer(0, &bufferInfo);
        writer.WriteImage(1, &info);

        writer.Build(skyboxDescriptorSet);
    }
}
