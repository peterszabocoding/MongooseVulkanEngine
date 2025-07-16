#include "renderer/vulkan/pass/lighting/brdf_lut_pass.h"

#include <renderer/mesh.h>
#include <renderer/vulkan/vulkan_pipeline.h>

namespace MongooseVK
{
    BrdfLUTPass::BrdfLUTPass(VulkanDevice* vulkanDevice, VkExtent2D _resolution): VulkanPass(vulkanDevice, _resolution)
    {
        screenRect = CreateScope<VulkanMeshlet>(device, Primitives::RECTANGLE_VERTICES, Primitives::RECTANGLE_INDICES);
        LoadPipeline();
    }

    void BrdfLUTPass::Render(VkCommandBuffer commandBuffer, Camera* camera, FramebufferHandle writeBufferHandle)
    {
        VulkanFramebuffer* framebuffer = device->GetFramebuffer(writeBufferHandle);

        device->SetViewportAndScissor(framebuffer->extent, commandBuffer);
        GetRenderPass()->Begin(commandBuffer, framebuffer->framebuffer, framebuffer->extent);

        DrawCommandParams drawCommandParams{};
        drawCommandParams.commandBuffer = commandBuffer;
        drawCommandParams.meshlet = screenRect.get();
        drawCommandParams.pipelineParams =
        {
            brdfLUTPipeline->GetPipeline(),
            brdfLUTPipeline->GetPipelineLayout()
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
        VulkanRenderPass::RenderPassConfig config;
        config.AddColorAttachment({.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT});

        renderPassHandle = device->CreateRenderPass(config);

        PipelineConfig iblBrdfPipelineConfig;
        iblBrdfPipelineConfig.vertexShaderPath = "brdf.vert";
        iblBrdfPipelineConfig.fragmentShaderPath = "brdf.frag";

        iblBrdfPipelineConfig.cullMode = PipelineCullMode::Front;
        iblBrdfPipelineConfig.colorAttachments = {
            ImageFormat::RGBA16_SFLOAT,
        };

        iblBrdfPipelineConfig.enableDepthTest = false;
        iblBrdfPipelineConfig.renderPass = GetRenderPass()->Get();

        brdfLUTPipeline = VulkanPipeline::Builder()
                .Build(device, iblBrdfPipelineConfig);
    }
}
