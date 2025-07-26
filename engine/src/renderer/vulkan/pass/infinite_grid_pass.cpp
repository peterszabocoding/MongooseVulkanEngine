#include "renderer/vulkan/pass/infinite_grid_pass.h"

#include "util/log.h"
#include "renderer/shader_cache.h"
#include "renderer/vulkan/vulkan_pipeline.h"
#include "renderer/vulkan/vulkan_mesh.h"

namespace MongooseVK
{
    InfiniteGridPass::InfiniteGridPass(VulkanDevice* vulkanDevice, VkExtent2D _resolution): VulkanPass(vulkanDevice, _resolution)
    {
        screenRect = CreateScope<VulkanMeshlet>(device, Primitives::GRID_VERTICES, Primitives::GRID_INDICES);
    }

    void InfiniteGridPass::Init()
    {
        VulkanPass::Init();
    }

    void InfiniteGridPass::Render(VkCommandBuffer commandBuffer, Camera* camera, FramebufferHandle writeBuffer)
    {
        VulkanFramebuffer* framebuffer = device->GetFramebuffer(framebufferHandle);

        device->SetViewportAndScissor(framebuffer->extent, commandBuffer);
        GetRenderPass()->Begin(commandBuffer, framebuffer->framebuffer, framebuffer->extent);

        DrawCommandParams screenRectDrawParams{};
        screenRectDrawParams.commandBuffer = commandBuffer;
        screenRectDrawParams.meshlet = screenRect.get();
        screenRectDrawParams.pushConstantParams.data = &gridParams;
        screenRectDrawParams.pushConstantParams.size = sizeof(GridParams);

        screenRectDrawParams.pipelineParams = {
            pipeline->pipeline,
            pipeline->pipelineLayout
        };

        screenRectDrawParams.descriptorSets = {
            device->bindlessTextureDescriptorSet,
            device->materialDescriptorSet,
            passDescriptorSet
        };

        device->DrawMeshlet(screenRectDrawParams);
        GetRenderPass()->End(commandBuffer);
    }

    void InfiniteGridPass::LoadPipeline()
    {
        LOG_TRACE("Building grid pipeline");

        pipelineConfig.vertexShaderPath = "infinite_grid.vert";
        pipelineConfig.fragmentShaderPath = "infinite_grid.frag";
        pipelineConfig.cullMode = PipelineCullMode::None;
        pipelineConfig.disableBlending = false;
        pipelineConfig.depthWriteEnable = false;
        pipelineConfig.renderPass = GetRenderPass()->Get();
        pipelineConfig.pushConstantData.shaderStageBits = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pipelineConfig.pushConstantData.size = sizeof(GridParams);
        pipelineConfig.descriptorSetLayouts = {
            device->bindlessTexturesDescriptorSetLayoutHandle,
            device->materialsDescriptorSetLayoutHandle,
            passDescriptorSetLayoutHandle
        };

        pipelineHandle = VulkanPipelineBuilder().Build(device, pipelineConfig);
        pipeline = device->GetPipeline(pipelineHandle);
    }
}
