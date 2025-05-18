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

    void PresentPass::Render(VkCommandBuffer commandBuffer, uint32_t imageIndex, Ref<VulkanFramebuffer> writeBuffer,
                             Ref<VulkanFramebuffer> readBuffer)
    {
        VkExtent2D extent{passWidth, passHeight};

        device->SetViewportAndScissor(extent, commandBuffer);
        renderPass->Begin(commandBuffer, writeBuffer, extent);

        DrawCommandParams screenRectDrawParams{};
        screenRectDrawParams.commandBuffer = commandBuffer;

        screenRectDrawParams.pipelineParams = {
            presentPipeline->GetPipeline(),
            presentPipeline->GetPipelineLayout()
        };

        screenRectDrawParams.descriptorSets = {
            ShaderCache::descriptorSets.presentDescriptorSets[imageIndex],
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
            presentPipelineConfig.vertexShaderPath = "shader/spv/quad.vert.spv";
            presentPipelineConfig.fragmentShaderPath = "shader/spv/quad.frag.spv";

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
