#include "renderer/vulkan/pass/post_processing/tone_mapping_pass.h"

#include <renderer/mesh.h>
#include <renderer/vulkan/vulkan_framebuffer.h>

namespace MongooseVK
{
    ToneMappingPass::ToneMappingPass(VulkanDevice* _device, VkExtent2D _resolution): FrameGraphRenderPass(_device, _resolution)
    {
        screenRect = CreateScope<VulkanMesh>(device);
        screenRect->AddMeshlet(Primitives::RECTANGLE_VERTICES, Primitives::RECTANGLE_INDICES);
    }

    ToneMappingPass::~ToneMappingPass() {}

    void ToneMappingPass::Render(VkCommandBuffer commandBuffer, SceneGraph* scene)
    {
        VulkanFramebuffer* framebuffer = device->GetFramebuffer(framebufferHandles[0]);

        device->SetViewportAndScissor(framebuffer->extent, commandBuffer);
        VulkanRenderPass* renderPass = device->renderPassPool.Get(renderPassHandle.handle);

        renderPass->Begin(commandBuffer, framebuffer->framebuffer, framebuffer->extent);

        DrawCommandParams drawParams{};
        drawParams.commandBuffer = commandBuffer;
        drawParams.meshlet = &screenRect->GetMeshlets()[0];

        drawParams.pipelineHandle = pipelineHandle;

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

    void ToneMappingPass::LoadPipeline(PipelineCreateInfo& pipelineCreate)
    {
        pipelineCreate.name = "ToneMappingPass";
        pipelineCreate.vertexShaderPath = "quad.vert";
        pipelineCreate.fragmentShaderPath = "post_processing_tone_mapping.frag";

        pipelineCreate.cullMode = PipelineCullMode::Front;
        pipelineCreate.enableDepthTest = false;

        pipelineCreate.descriptorSetLayouts = {
            device->bindlessTexturesDescriptorSetLayoutHandle,
            device->materialsDescriptorSetLayoutHandle,
            passDescriptorSetLayoutHandle
        };

        pipelineCreate.pushConstantData.shaderStageBits = VK_SHADER_STAGE_FRAGMENT_BIT;
        pipelineCreate.pushConstantData.size = sizeof(ToneMappingParams);
    }
}
