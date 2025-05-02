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

    void VulkanRenderer::Init(const int width, const int height)
    {
        viewportWidth = width;
        viewportHeight = height;

        LOG_TRACE("VulkanRenderer::Init()");
        vulkanDevice = CreateScope<VulkanDevice>(width, height, glfwWindow);

        CreateSwapchain();
        CreateRenderPasses();
        CreateFramebuffers();


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
                .Build();

        LOG_TRACE("Build pipelines");
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
                ImageFormat::RGBA8_UNORM,
            };

            geometryPipelineConfig.disableBlending = true;
            geometryPipelineConfig.enableDepthTest = true;
            geometryPipelineConfig.depthAttachment = ImageFormat::DEPTH24_STENCIL8;

            geometryPipelineConfig.renderPass = gBufferPass;

            geometryPipelineConfig.pushConstantData.shaderStageBits = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            geometryPipelineConfig.pushConstantData.offset = 0;
            geometryPipelineConfig.pushConstantData.size = sizeof(SimplePushConstantData);
        }
        pipelines.geometryPipeline = VulkanPipeline::Builder().Build(vulkanDevice.get(), geometryPipelineConfig);

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
            lightingPipelineConfig.renderPass = lightingPass;
        }
        pipelines.lightingPipeline = VulkanPipeline::Builder().Build(vulkanDevice.get(), lightingPipelineConfig);

        PrepareLightingPass();

        screenRect = CreateScope<VulkanMeshlet>(vulkanDevice.get(), Primitives::RECTANGLE_VERTICES, Primitives::RECTANGLE_INDICES);
        cube = CreateScope<VulkanMeshlet>(vulkanDevice.get(), Primitives::CUBE_VERTICES, Primitives::CUBE_INDICES);

        LOG_TRACE("Load skybox");
        //auto cubeMapTexture = ResourceManager::LoadHDRCubeMap(vulkanDevice.get(), "resources/environment/aristea_wreck_puresky_4k.hdr");

        LOG_TRACE("Load scene");
        completeScene = ResourceManager::LoadScene(vulkanDevice.get(), "resources/sponza/Sponza.gltf");
        //completeScene = ResourceManager::LoadScene(vulkanDevice.get(), "resources/gltf/multiple_spheres.gltf");
        //completeScene = ResourceManager::LoadScene(vulkanDevice.get(), "resources/chess/ABeautifulGame.gltf");

        transform.m_Position = glm::vec3(0.0f, 0.0f, -1.0f);
    }

    void VulkanRenderer::DrawGeometryPass(float deltaTime, Ref<Camera> camera, VkCommandBuffer commandBuffer)
    {
        gBufferPass->Begin(commandBuffer, gbufferFramebuffers[activeImage], vulkanSwapChain->GetExtent());
        for (size_t i = 0; i < completeScene.meshes.size(); i++)
        {
            vulkanDevice->DrawMesh(camera,
                                   completeScene.transforms[i],
                                   completeScene.meshes[i],
                                   pipelines.geometryPipeline);
        }
        gBufferPass->End(commandBuffer);
    }

    void VulkanRenderer::DrawLightingPass(VkCommandBuffer commandBuffer)
    {
        lightingPass->Begin(commandBuffer, presentFramebuffers[activeImage],
                            vulkanSwapChain->GetExtent());

        vulkanDevice->DrawMeshlet(*screenRect, pipelines.lightingPipeline->GetPipeline(),
                                  pipelines.lightingPipeline->GetPipelineLayout(),
                                  presentDescriptorSets[activeImage]);

        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

        lightingPass->End(commandBuffer);
    }

    void VulkanRenderer::DrawFrame(float deltaTime, Ref<Camera> camera)
    {
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
                    .SetRenderpass(gBufferPass)
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
                    .SetRenderpass(lightingPass)
                    .SetResolution(viewportWidth, viewportHeight)
                    .AddAttachment(vulkanSwapChain->GetImageViews()[i])
                    .Build();
        }

        for (size_t i = 0; i < 6; i++)
        {
            cubeMapFramebuffers[i] = VulkanFramebuffer::Builder(vulkanDevice.get())
                    .SetRenderpass(cubeMapPass)
                    .SetResolution(512, 512)
                    .AddAttachment(ImageFormat::RGBA16_SFLOAT)
                    .Build();
        }
    }

    void VulkanRenderer::CreateRenderPasses()
    {
        LOG_TRACE("Vulkan: create renderpass");
        gBufferPass = VulkanRenderPass::Builder(vulkanDevice.get())
                .AddColorAttachment(VK_FORMAT_R8G8B8A8_UNORM)
                .AddColorAttachment(VK_FORMAT_R8G8B8A8_UNORM)
                .AddColorAttachment(VK_FORMAT_R8G8B8A8_UNORM)
                .AddDepthAttachment()
                .Build();

        lightingPass = VulkanRenderPass::Builder(vulkanDevice.get())
                .AddColorAttachment(VK_FORMAT_R8G8B8A8_UNORM)
                .Build();

        cubeMapPass = VulkanRenderPass::Builder(vulkanDevice.get())
                .AddColorAttachment(VK_FORMAT_R8G8B8A8_UNORM)
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

    void VulkanRenderer::PrepareLightingPass()
    {
        presentSampler = ImageSamplerBuilder(vulkanDevice.get()).Build();

        VulkanDescriptorWriter writer = VulkanDescriptorWriter(*pipelines.lightingPipeline->GetDescriptorSetLayout(),
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

            writer.Build(presentDescriptorSets[i]);
        }
    }
}
