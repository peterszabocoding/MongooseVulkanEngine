#include "renderer/vulkan/pass/present_pass.h"

#include <backends/imgui_impl_vulkan.h>

#include "imgui.h"
#include "util/log.h"
#include "renderer/shader_cache.h"
#include "renderer/vulkan/vulkan_mesh.h"

namespace MongooseVK
{
    PresentPass::PresentPass(VulkanDevice* vulkanDevice, VkExtent2D _resolution): VulkanPass(vulkanDevice, _resolution)
    {
        screenRect = CreateScope<VulkanMeshlet>(device, Primitives::RECTANGLE_VERTICES, Primitives::RECTANGLE_INDICES);
        LoadPipeline();
    }

    void PresentPass::Render(VkCommandBuffer commandBuffer, Camera* camera, FramebufferHandle writeBuffer)
    {
        VulkanFramebuffer* framebuffer = device->GetFramebuffer(writeBuffer);

        device->SetViewportAndScissor(framebuffer->extent, commandBuffer);
        GetRenderPass()->Begin(commandBuffer, framebuffer->framebuffer, framebuffer->extent);

        DrawCommandParams screenRectDrawParams{};
        screenRectDrawParams.commandBuffer = commandBuffer;

        screenRectDrawParams.pipelineParams = {
            pipeline->pipeline,
            pipeline->pipelineLayout
        };

        screenRectDrawParams.descriptorSets = {
            ShaderCache::descriptorSets.presentDescriptorSet,
        };
        screenRectDrawParams.meshlet = screenRect.get();

        device->DrawMeshlet(screenRectDrawParams);
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

        GetRenderPass()->End(commandBuffer);
    }

    void PresentPass::CreateFramebuffer() {}

    void PresentPass::LoadPipeline()
    {
        LOG_TRACE("Building present pipeline");
        VulkanRenderPass::RenderPassConfig config;
        config.AddColorAttachment({
            .imageFormat = VK_FORMAT_R8G8B8A8_UNORM,
            .isSwapchainAttachment = true
        });

        renderPassHandle = device->CreateRenderPass(config);

        PipelineCreate pipelineConfig;
        pipelineConfig.name = "present_pipeline";
        pipelineConfig.vertexShaderPath = "quad.vert";
        pipelineConfig.fragmentShaderPath = "quad.frag";

       pipelineConfig.cullMode = PipelineCullMode::Front;
       pipelineConfig.polygonMode = PipelinePolygonMode::Fill;
       pipelineConfig.frontFace = PipelineFrontFace::Counter_clockwise;

        pipelineConfig.descriptorSetLayouts = {
            ShaderCache::descriptorSetLayouts.presentDescriptorSetLayout
        };

        pipelineConfig.disableBlending = true;
        pipelineConfig.enableDepthTest = false;

        pipelineConfig.renderPass = GetRenderPass()->Get();

        pipelineConfig.colorAttachments = {
            ImageFormat::RGBA8_UNORM,
        };

        pipelineHandle = VulkanPipelineBuilder().Build(device, pipelineConfig);
        pipeline = device->GetPipeline(pipelineHandle);
    }
}
