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

    void ToneMappingPass::Setup(FrameGraph* frameGraph)
    {
        FrameGraphResourceCreate resourceCreate{};
        resourceCreate.name = "main_frame_color";
        resourceCreate.type = FrameGraphResourceType::Texture;
        resourceCreate.resourceInfo.texture.textureCreateInfo = {
            .width = resolution.width,
            .height = resolution.height,
            .format = ImageFormat::RGBA8_UNORM,
            .addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        };

        frameGraph->CreateResource(resourceCreate);
        frameGraph->WriteResource("hdr_image", RenderPassOperation::LoadOp::Load, RenderPassOperation::StoreOp::Store);
    }

    void ToneMappingPass::Render(VkCommandBuffer commandBuffer, Scene* scene)
    {
        VulkanFramebuffer* framebuffer = device->GetFramebuffer(framebufferHandles[0]);

        device->SetViewportAndScissor(framebuffer->extent, commandBuffer);
        VulkanRenderPass* renderPass = device->renderPassPool.Get(renderPassHandle.handle);

        renderPass->Begin(commandBuffer, framebuffer->framebuffer, framebuffer->extent);

        DrawCommandParams drawParams{};
        drawParams.commandBuffer = commandBuffer;
        drawParams.meshlet = screenRect.get();

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

    void ToneMappingPass::Resize(VkExtent2D _resolution)
    {
        FrameGraphRenderPass::Resize(_resolution);
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
