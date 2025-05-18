#include "shadow_map_pass.h"

#include "renderer/shader_cache.h"
#include "util/log.h"

namespace Raytracing
{
    ShadowMapPass::ShadowMapPass(VulkanDevice* vulkanDevice, Scene& scene): VulkanPass(vulkanDevice), scene(scene)
    {
        renderPass = VulkanRenderPass::Builder(vulkanDevice)
                .AddDepthAttachment(VK_FORMAT_D32_SFLOAT)
                .Build();
        LoadPipelines();
    }

    void ShadowMapPass::Render(VkCommandBuffer commandBuffer, uint32_t imageIndex, Ref<VulkanFramebuffer> writeBuffer,
        Ref<VulkanFramebuffer> readBuffer)
    {
        VkExtent2D extent = {4096, 4096};

        device->SetViewportAndScissor(extent, commandBuffer);
        renderPass->Begin(commandBuffer, writeBuffer, extent);

        DrawCommandParams geometryDrawParams{};
        geometryDrawParams.commandBuffer = commandBuffer;

        geometryDrawParams.pipelineParams = {
            directionalShadowMapPipeline->GetPipeline(),
            directionalShadowMapPipeline->GetPipelineLayout()
        };

        for (size_t i = 0; i < scene.meshes.size(); i++)
        {
            SimplePushConstantData pushConstantData;
            pushConstantData.modelMatrix = scene.transforms[i].GetTransform();

            geometryDrawParams.pushConstantParams = {
                &pushConstantData,
                sizeof(SimplePushConstantData)
            };

            for (auto& meshlet: scene.meshes[i]->GetMeshlets())
            {
                geometryDrawParams.descriptorSets = {
                    ShaderCache::descriptorSets.lightsDescriptorSets[imageIndex]
                };
                geometryDrawParams.meshlet = &meshlet;
                device->DrawMeshlet(geometryDrawParams);
            }
        }

        renderPass->End(commandBuffer);
    }

    void ShadowMapPass::LoadPipelines()
    {
        LOG_TRACE("Building directional shadow map pipeline");
        PipelineConfig dirShadowMapPipelineConfig; {
            dirShadowMapPipelineConfig.vertexShaderPath = "shader/spv/shadow_map.vert.spv";
            dirShadowMapPipelineConfig.fragmentShaderPath = "shader/spv/empty.frag.spv";

            dirShadowMapPipelineConfig.cullMode = PipelineCullMode::Front;
            dirShadowMapPipelineConfig.polygonMode = PipelinePolygonMode::Fill;
            dirShadowMapPipelineConfig.frontFace = PipelineFrontFace::Counter_clockwise;

            dirShadowMapPipelineConfig.descriptorSetLayouts = {
                ShaderCache::descriptorSetLayouts.lightsDescriptorSetLayout,
            };

            dirShadowMapPipelineConfig.disableBlending = true;
            dirShadowMapPipelineConfig.enableDepthTest = true;
            dirShadowMapPipelineConfig.depthAttachment = ImageFormat::DEPTH32;

            dirShadowMapPipelineConfig.renderPass = renderPass;

            dirShadowMapPipelineConfig.pushConstantData.shaderStageBits = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            dirShadowMapPipelineConfig.pushConstantData.offset = 0;
            dirShadowMapPipelineConfig.pushConstantData.size = sizeof(SimplePushConstantData);
        }
        directionalShadowMapPipeline = VulkanPipeline::Builder().Build(device, dirShadowMapPipelineConfig);
    }
}
