#include "infinite_grid_pass.h"

#include "renderer/shader_cache.h"
#include "renderer/vulkan/vulkan_pipeline.h"
#include "util/log.h"

namespace Raytracing
{
    InfiniteGridPass::InfiniteGridPass(VulkanDevice* vulkanDevice, VkExtent2D _resolution): VulkanPass(vulkanDevice, _resolution)
    {
        VulkanRenderPass::ColorAttachment colorAttachment;
        colorAttachment.imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
        colorAttachment.loadOperation = VK_ATTACHMENT_LOAD_OP_LOAD;

        renderPass = VulkanRenderPass::Builder(vulkanDevice)
                .AddColorAttachment(colorAttachment)
                .AddDepthAttachment({VK_FORMAT_D24_UNORM_S8_UINT, VK_ATTACHMENT_LOAD_OP_LOAD})
                .Build();
        screenRect = CreateScope<VulkanMeshlet>(device, Primitives::GRID_VERTICES, Primitives::GRID_INDICES);
        LoadPipelines();
    }

    void InfiniteGridPass::Render(VkCommandBuffer commandBuffer, Camera& camera, Ref<VulkanFramebuffer> writeBuffer,
                                  Ref<VulkanFramebuffer> readBuffer)
    {
        device->SetViewportAndScissor(writeBuffer->GetExtent(), commandBuffer);
        renderPass->Begin(commandBuffer, writeBuffer, writeBuffer->GetExtent());

        DrawCommandParams screenRectDrawParams{};
        screenRectDrawParams.commandBuffer = commandBuffer;
        screenRectDrawParams.meshlet = screenRect.get();
        screenRectDrawParams.pushConstantParams.data = &gridParams;
        screenRectDrawParams.pushConstantParams.size = sizeof(GridParams);

        screenRectDrawParams.pipelineParams = {
            gridPipeline->GetPipeline(),
            gridPipeline->GetPipelineLayout()
        };

        screenRectDrawParams.descriptorSets = {
            ShaderCache::descriptorSets.transformDescriptorSet,
        };

        device->DrawMeshlet(screenRectDrawParams);
        renderPass->End(commandBuffer);
    }

    void InfiniteGridPass::LoadPipelines()
    {
        LOG_TRACE("Building present pipeline");
        PipelineConfig pipelineConfig; {
            pipelineConfig.vertexShaderPath = "infinite_grid.vert";
            pipelineConfig.fragmentShaderPath = "infinite_grid.frag";

            pipelineConfig.cullMode = PipelineCullMode::Unknown;
            pipelineConfig.polygonMode = PipelinePolygonMode::Fill;
            pipelineConfig.frontFace = PipelineFrontFace::Counter_clockwise;
            pipelineConfig.disableBlending = false;
            pipelineConfig.enableDepthTest = true;
            pipelineConfig.renderPass = renderPass;
            pipelineConfig.pushConstantData.shaderStageBits = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            pipelineConfig.pushConstantData.size = sizeof(GridParams);
            pipelineConfig.depthAttachment = ImageFormat::DEPTH24_STENCIL8;

            pipelineConfig.descriptorSetLayouts = {
                ShaderCache::descriptorSetLayouts.transformDescriptorSetLayout
            };

            pipelineConfig.colorAttachments = {
                ImageFormat::RGBA8_UNORM,
            };
        }
        gridPipeline = VulkanPipeline::Builder().Build(device, pipelineConfig);
    }
}
