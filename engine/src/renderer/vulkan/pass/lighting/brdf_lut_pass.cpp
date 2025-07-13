#include "renderer/vulkan/pass/lighting/brdf_lut_pass.h"

#include <renderer/mesh.h>

namespace MongooseVK
{
    BrdfLUTPass::BrdfLUTPass(VulkanDevice* vulkanDevice, VkExtent2D _resolution): VulkanPass(vulkanDevice, _resolution)
    {
        screenRect = CreateScope<VulkanMeshlet>(device, Primitives::RECTANGLE_VERTICES, Primitives::RECTANGLE_INDICES);
        LoadPipeline();
    }

    void BrdfLUTPass::Render(VkCommandBuffer commandBuffer, Camera* camera, Ref<VulkanFramebuffer> writeBuffer,
                             Ref<VulkanFramebuffer> readBuffer)
    {
        VkExtent2D extent = {writeBuffer->GetWidth(), writeBuffer->GetHeight()};

        device->SetViewportAndScissor(extent, commandBuffer);
        GetRenderPass()->Begin(commandBuffer, writeBuffer, extent);

        DrawCommandParams drawCommandParams{};
        drawCommandParams.commandBuffer = commandBuffer;

        drawCommandParams.pipelineParams =
        {
            brdfLUTPipeline->GetPipeline(),
            brdfLUTPipeline->GetPipelineLayout()
        };

        drawCommandParams.meshlet = screenRect.get();

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
        iblBrdfPipelineConfig.polygonMode = PipelinePolygonMode::Fill;
        iblBrdfPipelineConfig.frontFace = PipelineFrontFace::Counter_clockwise;

        iblBrdfPipelineConfig.descriptorSetLayouts = {};

        iblBrdfPipelineConfig.colorAttachments = {
            ImageFormat::RGBA16_SFLOAT,
        };

        iblBrdfPipelineConfig.disableBlending = true;
        iblBrdfPipelineConfig.enableDepthTest = false;

        iblBrdfPipelineConfig.renderPass = GetRenderPass()->Get();

        brdfLUTPipeline = VulkanPipeline::Builder()
                .Build(device, iblBrdfPipelineConfig);
    }
}
