#include "renderer/vulkan/pass/lighting_pass.h"

#include <renderer/vulkan/vulkan_framebuffer.h>
#include <renderer/vulkan/vulkan_mesh.h>

#include "renderer/shader_cache.h"
#include "resource/resource_manager.h"
#include "util/log.h"

namespace MongooseVK
{
    LightingPass::LightingPass(VulkanDevice* vulkanDevice, VkExtent2D _resolution): FrameGraphRenderPass(vulkanDevice, _resolution) {}

    void LightingPass::Setup(FrameGraph* frameGraph)
    {
        frameGraph->WriteResource("hdr_image", RenderPassOperation::LoadOp::Load, RenderPassOperation::StoreOp::Store);
        frameGraph->WriteResource("depth_map", RenderPassOperation::LoadOp::Load, RenderPassOperation::StoreOp::Store);

        frameGraph->ReadResource("directional_shadow_map");
        frameGraph->ReadResource("ssao_texture");
        frameGraph->ReadResource("prefilter_map_texture");
        frameGraph->ReadResource("brdflut_texture");
    }

    void LightingPass::Render(VkCommandBuffer commandBuffer, Scene* scene)
    {
        VulkanFramebuffer* framebuffer = device->GetFramebuffer(framebufferHandles[0]);

        device->SetViewportAndScissor(framebuffer->extent, commandBuffer);
        GetRenderPass()->Begin(commandBuffer, framebuffer->framebuffer, framebuffer->extent);

        DrawCommandParams drawCommandParams{};
        drawCommandParams.commandBuffer = commandBuffer;
        drawCommandParams.pipelineHandle = pipelineHandle;

        for (size_t i = 0; i < scene->meshes.size(); i++)
        {
            for (auto& meshlet: scene->meshes[i]->GetMeshlets())
            {
                VulkanMaterial* material = device->GetMaterial(scene->meshes[i]->GetMaterial(meshlet));

                SimplePushConstantData pushConstantData;
                pushConstantData.modelMatrix = scene->transforms[i].GetTransform();
                pushConstantData.materialIndex = material->index;

                drawCommandParams.pushConstantParams = {
                    &pushConstantData,
                    sizeof(SimplePushConstantData)
                };

                drawCommandParams.meshlet = &meshlet;
                drawCommandParams.descriptorSets = {
                    device->bindlessTextureDescriptorSet,
                    device->materialDescriptorSet,
                    passDescriptorSet
                };

                device->DrawMeshlet(drawCommandParams);
            }
        }

        GetRenderPass()->End(commandBuffer);
    }

    void LightingPass::LoadPipeline(PipelineCreateInfo& pipelineCreate)
    {
        pipelineCreate.name = "LightingPass";
        pipelineCreate.vertexShaderPath = "base-pass.vert";
        pipelineCreate.fragmentShaderPath = "lighting-pass.frag";

        pipelineCreate.descriptorSetLayouts = {
            device->bindlessTexturesDescriptorSetLayoutHandle,
            device->materialsDescriptorSetLayoutHandle,
            passDescriptorSetLayoutHandle
        };

        pipelineCreate.disableBlending = false;

        pipelineCreate.pushConstantData.shaderStageBits = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pipelineCreate.pushConstantData.size = sizeof(SimplePushConstantData);
    }
}
