#include "renderer/vulkan/pass/gbufferPass.h"

#include <random>
#include <glm/gtc/packing.inl>
#include <renderer/vulkan/vulkan_mesh.h>

#include "renderer/shader_cache.h"
#include "renderer/vulkan/vulkan_pipeline.h"
#include "util/log.h"

namespace MongooseVK
{
    GBufferPass::GBufferPass(VulkanDevice* vulkanDevice, Scene& _scene, VkExtent2D _resolution): VulkanPass(vulkanDevice, _resolution),
        scene(_scene){}

    void GBufferPass::Init()
    {
        VulkanPass::Init();
    }

    void GBufferPass::Render(VkCommandBuffer commandBuffer)
    {
        VulkanFramebuffer* framebuffer = device->GetFramebuffer(framebufferHandles[0]);

        device->SetViewportAndScissor(resolution, commandBuffer);
        GetRenderPass()->Begin(commandBuffer, framebuffer->framebuffer, resolution);

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

                if (material->params.alphaTested) continue;

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

    void GBufferPass::Resize(VkExtent2D _resolution)
    {
        VulkanPass::Resize(_resolution);
    }

    void GBufferPass::LoadPipeline()
    {
        LOG_TRACE("Building gbuffer pipeline");
        constexpr PipelinePushConstantData pushConstantData = {
            .shaderStageBits = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            .offset = 0,
            .size = sizeof(SimplePushConstantData),
        };

        pipelineConfig.vertexShaderPath = "gbuffer.vert";
        pipelineConfig.fragmentShaderPath = "gbuffer.frag";

        pipelineConfig.descriptorSetLayouts = {
            device->bindlessTexturesDescriptorSetLayoutHandle,
            device->materialsDescriptorSetLayoutHandle,
            passDescriptorSetLayoutHandle
        };

        pipelineConfig.renderPass = GetRenderPass()->Get();
        pipelineConfig.pushConstantData = pushConstantData;

        pipelineHandle = VulkanPipelineBuilder().Build(device, pipelineConfig);
        pipeline = device->GetPipeline(pipelineHandle);
    }
}
