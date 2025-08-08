#include "renderer/vulkan/pass/lighting/brdf_lut_pass.h"

#include <renderer/mesh.h>
#include <renderer/vulkan/vulkan_framebuffer.h>
#include <renderer/vulkan/vulkan_pipeline.h>

namespace MongooseVK
{
    BrdfLUTPass::BrdfLUTPass(VulkanDevice* vulkanDevice, VkExtent2D _resolution): FrameGraphRenderPass(vulkanDevice, VkExtent2D{512, 512})
    {
        screenRect = CreateScope<VulkanMeshlet>(device, Primitives::RECTANGLE_VERTICES, Primitives::RECTANGLE_INDICES);
    }

    void BrdfLUTPass::Init()
    {
        FrameGraphRenderPass::Init();
    }

    void BrdfLUTPass::Render(VkCommandBuffer commandBuffer, Scene* scene)
    {
        const VulkanFramebuffer* framebuffer = device->GetFramebuffer(framebufferHandles[0]);

        device->SetViewportAndScissor(framebuffer->extent, commandBuffer);
        GetRenderPass()->Begin(commandBuffer, framebuffer->framebuffer, framebuffer->extent);

        DrawCommandParams drawCommandParams{};
        drawCommandParams.commandBuffer = commandBuffer;
        drawCommandParams.meshlet = screenRect.get();
        drawCommandParams.pipelineHandle = pipelineHandle;

        device->DrawMeshlet(drawCommandParams);

        GetRenderPass()->End(commandBuffer);
    }

    void BrdfLUTPass::Resize(VkExtent2D _resolution)
    {
        FrameGraphRenderPass::Resize(_resolution);
    }

    void BrdfLUTPass::LoadPipeline(PipelineCreateInfo& pipelineCreate)
    {
        pipelineCreate.name = "BrdfLUTPass";
        pipelineCreate.vertexShaderPath = "brdf.vert";
        pipelineCreate.fragmentShaderPath = "brdf.frag";

        pipelineCreate.cullMode = PipelineCullMode::Front;
        pipelineCreate.enableDepthTest = false;

        LOG_TRACE(pipelineCreate.name);
    }
}
