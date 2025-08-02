#include "renderer/vulkan/pass/lighting/brdf_lut_pass.h"

#include <renderer/mesh.h>
#include <renderer/vulkan/vulkan_pipeline.h>

namespace MongooseVK
{
    BrdfLUTPass::BrdfLUTPass(VulkanDevice* vulkanDevice, VkExtent2D _resolution): VulkanPass(vulkanDevice, _resolution)
    {
        screenRect = CreateScope<VulkanMeshlet>(device, Primitives::RECTANGLE_VERTICES, Primitives::RECTANGLE_INDICES);
    }

    void BrdfLUTPass::Init()
    {
        VulkanPass::Init();
    }

    void BrdfLUTPass::Render(VkCommandBuffer commandBuffer)
    {
        VulkanFramebuffer* framebuffer = device->GetFramebuffer(framebufferHandles[0]);

        device->SetViewportAndScissor(framebuffer->extent, commandBuffer);
        GetRenderPass()->Begin(commandBuffer, framebuffer->framebuffer, framebuffer->extent);

        DrawCommandParams drawCommandParams{};
        drawCommandParams.commandBuffer = commandBuffer;
        drawCommandParams.meshlet = screenRect.get();
        drawCommandParams.pipelineParams =
        {
            pipeline->pipeline,
            pipeline->pipelineLayout
        };

        device->DrawMeshlet(drawCommandParams);

        GetRenderPass()->End(commandBuffer);
    }

    void BrdfLUTPass::Resize(VkExtent2D _resolution)
    {
        VulkanPass::Resize(_resolution);
    }

    void BrdfLUTPass::LoadPipeline()
    {
        pipelineConfig.vertexShaderPath = "brdf.vert";
        pipelineConfig.fragmentShaderPath = "brdf.frag";

        pipelineConfig.cullMode = PipelineCullMode::Front;
        pipelineConfig.enableDepthTest = false;
        pipelineConfig.renderPass = GetRenderPass()->Get();

        pipelineHandle = VulkanPipelineBuilder().Build(device, pipelineConfig);
        pipeline = device->GetPipeline(pipelineHandle);
    }
}
