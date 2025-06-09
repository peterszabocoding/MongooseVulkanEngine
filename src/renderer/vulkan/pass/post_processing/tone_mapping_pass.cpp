#include "tone_mapping_pass.h"

#include "renderer/shader_cache.h"
#include "renderer/vulkan/vulkan_pipeline.h"

namespace Raytracing
{
    ToneMappingPass::ToneMappingPass(VulkanDevice* _device, VkExtent2D _resolution): VulkanPass(_device, _resolution) {}
    ToneMappingPass::~ToneMappingPass() {}

    void ToneMappingPass::Render(VkCommandBuffer commandBuffer, Camera& camera, Ref<VulkanFramebuffer> writeBuffer,
                                 Ref<VulkanFramebuffer> readBuffer) {}

    void ToneMappingPass::LoadPipeline()
    {
        LOG_TRACE("Building tone mapping pipeline");
        PipelineConfig pipelineConfig;
        pipelineConfig.vertexShaderPath = "quad.vert";
        pipelineConfig.fragmentShaderPath = "post_processing_tone_mapping.frag";

        pipelineConfig.cullMode = PipelineCullMode::Front;
        pipelineConfig.polygonMode = PipelinePolygonMode::Fill;
        pipelineConfig.frontFace = PipelineFrontFace::Counter_clockwise;

        pipelineConfig.descriptorSetLayouts = {
            ShaderCache::descriptorSetLayouts.ssaoDescriptorSetLayout,
        };

        pipelineConfig.colorAttachments = {
            ImageFormat::R8_UNORM,
        };

        pipelineConfig.disableBlending = true;
        pipelineConfig.enableDepthTest = false;

        pipelineConfig.renderPass = renderPass;

        pipeline = VulkanPipeline::Builder().Build(device, pipelineConfig);
    }
    void ToneMappingPass::InitDescriptorSet() {}
}
