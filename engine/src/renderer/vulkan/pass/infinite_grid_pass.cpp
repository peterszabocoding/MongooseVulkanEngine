#include "renderer/vulkan/pass/infinite_grid_pass.h"
#include <renderer/vulkan/vulkan_framebuffer.h>

#include "util/log.h"
#include "renderer/vulkan/vulkan_pipeline.h"
#include "renderer/vulkan/vulkan_mesh.h"

namespace MongooseVK
{
    InfiniteGridPass::InfiniteGridPass(VulkanDevice* vulkanDevice, VkExtent2D _resolution): FrameGraphRenderPass(vulkanDevice, _resolution)
    {
        screenRect = CreateScope<VulkanMesh>(device);
        screenRect->AddMeshlet(Primitives::GRID_VERTICES, Primitives::GRID_INDICES);
    }

    void InfiniteGridPass::Init()
    {
        FrameGraphRenderPass::Init();
    }

    void InfiniteGridPass::Render(VkCommandBuffer commandBuffer, SceneGraph* scene)
    {
        const VulkanFramebuffer* framebuffer = device->GetFramebuffer(framebufferHandles[0]);

        device->SetViewportAndScissor(framebuffer->extent, commandBuffer);
        GetRenderPass()->Begin(commandBuffer, framebuffer->framebuffer, framebuffer->extent);

        DrawCommandParams drawCommandParams{};
        drawCommandParams.commandBuffer = commandBuffer;
        drawCommandParams.meshlet = &screenRect->GetMeshlets()[0];
        drawCommandParams.pushConstantParams.data = &gridParams;
        drawCommandParams.pushConstantParams.size = sizeof(GridParams);

        drawCommandParams.pipelineHandle = pipelineHandle;
        drawCommandParams.descriptorSets = {
            device->bindlessTextureDescriptorSet,
            device->materialDescriptorSet,
            passDescriptorSet
        };

        device->DrawMeshlet(drawCommandParams);
        GetRenderPass()->End(commandBuffer);
    }

    void InfiniteGridPass::LoadPipeline(PipelineCreateInfo& pipelineCreate)
    {
        pipelineCreate.name = "InfiniteGridPass";
        pipelineCreate.vertexShaderPath = "infinite_grid.vert";
        pipelineCreate.fragmentShaderPath = "infinite_grid.frag";
        pipelineCreate.cullMode = PipelineCullMode::None;
        pipelineCreate.disableBlending = false;
        pipelineCreate.depthWriteEnable = false;
        pipelineCreate.pushConstantData.shaderStageBits = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pipelineCreate.pushConstantData.size = sizeof(GridParams);
        pipelineCreate.descriptorSetLayouts = {
            device->bindlessTexturesDescriptorSetLayoutHandle,
            device->materialsDescriptorSetLayoutHandle,
            passDescriptorSetLayoutHandle
        };
    }
}
