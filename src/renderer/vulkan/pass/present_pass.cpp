#include "present_pass.h"

#include <backends/imgui_impl_vulkan.h>

#include "imgui.h"
#include "renderer/shader_cache.h"
#include "util/log.h"

namespace Raytracing
{
    PresentPass::PresentPass(VulkanDevice* vulkanDevice): VulkanPass(vulkanDevice)
    {
        renderPass = VulkanRenderPass::Builder(vulkanDevice)
                .AddColorAttachment(VK_FORMAT_R8G8B8A8_UNORM, true)
                .Build();
        screenRect = CreateScope<VulkanMeshlet>(device, Primitives::RECTANGLE_VERTICES, Primitives::RECTANGLE_INDICES);
        LoadPipeline();
    }

    void PresentPass::Render(VkCommandBuffer commandBuffer, Camera& camera, Ref<VulkanFramebuffer> writeBuffer,
                             Ref<VulkanFramebuffer> readBuffer)
    {
        device->SetViewportAndScissor(writeBuffer->GetExtent(), commandBuffer);
        renderPass->Begin(commandBuffer, writeBuffer, writeBuffer->GetExtent());

        DrawCommandParams screenRectDrawParams{};
        screenRectDrawParams.commandBuffer = commandBuffer;

        screenRectDrawParams.pipelineParams = {
            presentPipeline->GetPipeline(),
            presentPipeline->GetPipelineLayout()
        };

        screenRectDrawParams.descriptorSets = {
            ShaderCache::descriptorSets.presentDescriptorSet,
        };
        screenRectDrawParams.meshlet = screenRect.get();

        device->DrawMeshlet(screenRectDrawParams);
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

        renderPass->End(commandBuffer);
    }

    void PresentPass::LoadPipeline()
    {
        LOG_TRACE("Building present pipeline");
        PipelineConfig presentPipelineConfig; {
            presentPipelineConfig.vertexShaderPath = "quad.vert";
            presentPipelineConfig.fragmentShaderPath = "quad.frag";

            presentPipelineConfig.cullMode = PipelineCullMode::Front;
            presentPipelineConfig.polygonMode = PipelinePolygonMode::Fill;
            presentPipelineConfig.frontFace = PipelineFrontFace::Counter_clockwise;

            presentPipelineConfig.descriptorSetLayouts = {
                ShaderCache::descriptorSetLayouts.presentDescriptorSetLayout
            };

            presentPipelineConfig.colorAttachments = {
                ImageFormat::RGBA8_UNORM,
            };

            presentPipelineConfig.disableBlending = true;
            presentPipelineConfig.enableDepthTest = false;

            presentPipelineConfig.renderPass = renderPass;
        }
        presentPipeline = VulkanPipeline::Builder().Build(device, presentPipelineConfig);
    }
}
