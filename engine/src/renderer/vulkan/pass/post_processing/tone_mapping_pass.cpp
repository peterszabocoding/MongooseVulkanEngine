#include "renderer/vulkan/pass/post_processing/tone_mapping_pass.h"

#include <renderer/mesh.h>
#include <renderer/vulkan/vulkan_framebuffer.h>

namespace MongooseVK
{
    ToneMappingPass::ToneMappingPass(VulkanDevice* _device, VkExtent2D _resolution): FrameGraphRenderPass(_device, _resolution)
    {
        screenRect = CreateScope<VulkanMeshlet>(device, Primitives::RECTANGLE_VERTICES, Primitives::RECTANGLE_INDICES);
    }

    ToneMappingPass::~ToneMappingPass() {}

    void ToneMappingPass::Render(VkCommandBuffer commandBuffer, Scene* scene)
    {
        VulkanFramebuffer* framebuffer = device->GetFramebuffer(framebufferHandles[0]);

        device->SetViewportAndScissor(framebuffer->extent, commandBuffer);
        VulkanRenderPass* renderPass = device->renderPassPool.Get(renderPassHandle.handle);

        renderPass->Begin(commandBuffer, framebuffer->framebuffer, framebuffer->extent);

        DrawCommandParams drawParams{};
        drawParams.commandBuffer = commandBuffer;
        drawParams.meshlet = screenRect.get();

        drawParams.pipelineParams = {
            pipeline->pipeline,
            pipeline->pipelineLayout
        };

        drawParams.descriptorSets = {
            passDescriptorSet,
        };

        drawParams.pushConstantParams = {
            &toneMappingParams,
            sizeof(ToneMappingParams)
        };

        device->DrawMeshlet(drawParams);
        renderPass->End(commandBuffer);
    }

    void ToneMappingPass::Resize(VkExtent2D _resolution)
    {
        FrameGraphRenderPass::Resize(_resolution);
    }

    void ToneMappingPass::LoadPipeline()
    {
        LOG_TRACE("Building tone mapping pipeline");
        pipelineConfig.vertexShaderPath = "quad.vert";
        pipelineConfig.fragmentShaderPath = "post_processing_tone_mapping.frag";

        pipelineConfig.cullMode = PipelineCullMode::Front;
        pipelineConfig.enableDepthTest = false;
        pipelineConfig.renderPass = GetRenderPass()->Get();

        pipelineConfig.descriptorSetLayouts = {
            device->bindlessTexturesDescriptorSetLayoutHandle,
            device->materialsDescriptorSetLayoutHandle,
            passDescriptorSetLayoutHandle
        };


        pipelineConfig.pushConstantData.shaderStageBits = VK_SHADER_STAGE_FRAGMENT_BIT;
        pipelineConfig.pushConstantData.offset = 0;
        pipelineConfig.pushConstantData.size = sizeof(ToneMappingParams);


        pipelineHandle = VulkanPipelineBuilder().Build(device, pipelineConfig);
        pipeline = device->GetPipeline(pipelineHandle);
    }
}
