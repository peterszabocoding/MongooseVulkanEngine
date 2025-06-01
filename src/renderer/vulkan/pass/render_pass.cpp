#include "render_pass.h"

#include "renderer/shader_cache.h"
#include "resource/resource_manager.h"
#include "util/log.h"

namespace Raytracing
{
    RenderPass::RenderPass(VulkanDevice* vulkanDevice, Scene& _scene): VulkanPass(vulkanDevice), scene(_scene)
    {
        renderPass = VulkanRenderPass::Builder(vulkanDevice)
                .AddColorAttachment(VK_FORMAT_R8G8B8A8_UNORM, false, false)
                .AddDepthAttachment()
                .Build();
        LoadPipelines();
    }

    void RenderPass::Render(VkCommandBuffer commandBuffer, Camera& camera, uint32_t imageIndex, Ref<VulkanFramebuffer> writeBuffer,
                            Ref<VulkanFramebuffer> readBuffer)
    {
        device->SetViewportAndScissor(writeBuffer->GetExtent(), commandBuffer);
        renderPass->Begin(commandBuffer, writeBuffer, writeBuffer->GetExtent());

        DrawCommandParams geometryDrawParams{};
        geometryDrawParams.commandBuffer = commandBuffer;
        geometryDrawParams.pipelineParams =
        {
            geometryPipeline->GetPipeline(),
            geometryPipeline->GetPipelineLayout()
        };

        for (size_t i = 0; i < scene.meshes.size(); i++)
        {
            SimplePushConstantData pushConstantData;
            pushConstantData.modelMatrix = scene.transforms[i].GetTransform();
            pushConstantData.transform = camera.GetProjection() * camera.GetView() * pushConstantData.modelMatrix;

            geometryDrawParams.pushConstantParams = {
                &pushConstantData,
                sizeof(SimplePushConstantData)
            };

            for (auto& meshlet: scene.meshes[i]->GetMeshlets())
            {
                geometryDrawParams.descriptorSets = {
                    scene.meshes[i]->GetMaterial(meshlet).descriptorSet,
                    ShaderCache::descriptorSets.transformDescriptorSet,
                    ShaderCache::descriptorSets.lightsDescriptorSet,
                    ShaderCache::descriptorSets.irradianceDescriptorSet,
                    ShaderCache::descriptorSets.postProcessingDescriptorSet,
                };

                if (scene.reflectionProbe)
                {
                    geometryDrawParams.descriptorSets.push_back(scene.reflectionProbe->descriptorSet);
                }

                geometryDrawParams.meshlet = &meshlet;
                device->DrawMeshlet(geometryDrawParams);
            }
        }

        renderPass->End(commandBuffer);
    }

    void RenderPass::LoadPipelines()
    {
        LOG_TRACE("Building geometry pipeline");
        PipelineConfig geometryPipelineConfig; {
            geometryPipelineConfig.vertexShaderPath = "base-pass.vert";
            geometryPipelineConfig.fragmentShaderPath = "lighting-pass.frag";

            geometryPipelineConfig.cullMode = PipelineCullMode::Back;
            geometryPipelineConfig.polygonMode = PipelinePolygonMode::Fill;
            geometryPipelineConfig.frontFace = PipelineFrontFace::Counter_clockwise;

            geometryPipelineConfig.descriptorSetLayouts = {
                ShaderCache::descriptorSetLayouts.materialDescriptorSetLayout,
                ShaderCache::descriptorSetLayouts.transformDescriptorSetLayout,
                ShaderCache::descriptorSetLayouts.lightsDescriptorSetLayout,
                ShaderCache::descriptorSetLayouts.irradianceDescriptorSetLayout,
                ShaderCache::descriptorSetLayouts.postProcessingDescriptorSetLayout,
                ShaderCache::descriptorSetLayouts.reflectionDescriptorSetLayout,
            };

            geometryPipelineConfig.colorAttachments = {
                ImageFormat::RGBA8_UNORM,
            };

            geometryPipelineConfig.disableBlending = true;
            geometryPipelineConfig.enableDepthTest = true;
            geometryPipelineConfig.depthAttachment = ImageFormat::DEPTH24_STENCIL8;

            geometryPipelineConfig.renderPass = renderPass;

            geometryPipelineConfig.pushConstantData.shaderStageBits = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            geometryPipelineConfig.pushConstantData.offset = 0;
            geometryPipelineConfig.pushConstantData.size = sizeof(SimplePushConstantData);
        }
        geometryPipeline = VulkanPipeline::Builder().Build(device, geometryPipelineConfig);
    }
}
