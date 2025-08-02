#include "renderer/vulkan/pass/lighting_pass.h"

#include <renderer/vulkan/vulkan_mesh.h>

#include "renderer/shader_cache.h"
#include "resource/resource_manager.h"
#include "util/log.h"

namespace MongooseVK
{
    LightingPass::LightingPass(VulkanDevice* vulkanDevice, Scene& _scene, VkExtent2D _resolution): VulkanPass(vulkanDevice, _resolution),
        scene(_scene){}

    void LightingPass::Render(VkCommandBuffer commandBuffer)
    {
        VulkanFramebuffer* framebuffer = device->GetFramebuffer(framebufferHandles[0]);

        device->SetViewportAndScissor(framebuffer->extent, commandBuffer);
        GetRenderPass()->Begin(commandBuffer, framebuffer->framebuffer, framebuffer->extent);

        DrawCommandParams drawCommandParams{};
        drawCommandParams.commandBuffer = commandBuffer;
        drawCommandParams.pipelineParams =
        {
            pipeline->pipeline,
            pipeline->pipelineLayout
        };

        for (size_t i = 0; i < scene.meshes.size(); i++)
        {
            for (auto& meshlet: scene.meshes[i]->GetMeshlets())
            {
                VulkanMaterial* material = device->GetMaterial(scene.meshes[i]->GetMaterial(meshlet));

                SimplePushConstantData pushConstantData;
                pushConstantData.modelMatrix = scene.transforms[i].GetTransform();
                pushConstantData.materialIndex = material->index;

                drawCommandParams.pushConstantParams = {
                    &pushConstantData,
                    sizeof(SimplePushConstantData)
                };

                drawCommandParams.descriptorSets = {
                    device->bindlessTextureDescriptorSet,
                    device->materialDescriptorSet,
                    passDescriptorSet
                };

                drawCommandParams.meshlet = &meshlet;
                device->DrawMeshlet(drawCommandParams);
            }
        }

        GetRenderPass()->End(commandBuffer);
    }

    void LightingPass::LoadPipeline()
    {
        LOG_TRACE("Building lighting pipeline");
        pipelineConfig.vertexShaderPath = "base-pass.vert";
        pipelineConfig.fragmentShaderPath = "lighting-pass.frag";

        pipelineConfig.cullMode = PipelineCullMode::Back;

        pipelineConfig.descriptorSetLayouts = {
            device->bindlessTexturesDescriptorSetLayoutHandle,
            device->materialsDescriptorSetLayoutHandle,
            passDescriptorSetLayoutHandle
        };

        pipelineConfig.disableBlending = false;

        pipelineConfig.renderPass = GetRenderPass()->Get();

        pipelineConfig.pushConstantData.shaderStageBits = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pipelineConfig.pushConstantData.offset = 0;
        pipelineConfig.pushConstantData.size = sizeof(SimplePushConstantData);

        pipelineHandle = VulkanPipelineBuilder().Build(device, pipelineConfig);
        pipeline = device->GetPipeline(pipelineHandle);
    }
}
