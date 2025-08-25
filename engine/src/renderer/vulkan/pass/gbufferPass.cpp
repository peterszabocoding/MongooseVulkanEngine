#include "renderer/vulkan/pass/gbufferPass.h"

#include <random>
#include <glm/gtc/packing.inl>
#include <renderer/vulkan/vulkan_framebuffer.h>
#include <renderer/vulkan/vulkan_mesh.h>

#include "renderer/vulkan/vulkan_pipeline.h"

namespace MongooseVK
{
    GBufferPass::GBufferPass(VulkanDevice* vulkanDevice, VkExtent2D _resolution): FrameGraphRenderPass(vulkanDevice, _resolution) {}

    void GBufferPass::Render(VkCommandBuffer commandBuffer, SceneGraph* scene)
    {
        const VulkanFramebuffer* framebuffer = device->GetFramebuffer(framebufferHandles[0]);

        device->SetViewportAndScissor(resolution, commandBuffer);
        GetRenderPass()->Begin(commandBuffer, framebuffer->framebuffer, resolution);

        DrawCommandParams drawCommandParams{};
        drawCommandParams.commandBuffer = commandBuffer;
        drawCommandParams.pipelineHandle = pipelineHandle;

        for (size_t i = 0; i < scene->meshes.size(); i++)
        {
            if (!scene->meshes[i]) continue;

            for (auto& meshlet: scene->meshes[i]->GetMeshlets())
            {
                VulkanMaterial* material = device->GetMaterial(meshlet.material);

                if (material->params.alphaTested) continue;

                SimplePushConstantData pushConstantData{
                    .modelMatrix = scene->GetTransform(i).GetTransform(),
                    .materialIndex = material->index,
                };

                drawCommandParams.meshlet = &meshlet;
                drawCommandParams.pushConstantParams = {&pushConstantData, sizeof(SimplePushConstantData)};
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

    void GBufferPass::LoadPipeline(PipelineCreateInfo& pipelineCreate)
    {
        pipelineCreate.name = "GBufferPass";
        pipelineCreate.vertexShaderPath = "gbuffer.vert";
        pipelineCreate.fragmentShaderPath = "gbuffer.frag";

        pipelineCreate.descriptorSetLayouts = {
            device->bindlessTexturesDescriptorSetLayoutHandle,
            device->materialsDescriptorSetLayoutHandle,
            passDescriptorSetLayoutHandle
        };

        pipelineCreate.pushConstantData = {
            .shaderStageBits = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            .size = sizeof(SimplePushConstantData),
        };
    }
}
