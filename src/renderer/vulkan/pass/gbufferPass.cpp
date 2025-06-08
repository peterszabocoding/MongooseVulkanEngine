#include "gbufferPass.h"

#include <random>
#include <glm/gtc/packing.inl>

#include "renderer/shader_cache.h"
#include "renderer/vulkan/vulkan_pipeline.h"
#include "util/log.h"

namespace Raytracing
{
    GBufferPass::GBufferPass(VulkanDevice* vulkanDevice, Scene& _scene): VulkanPass(vulkanDevice), scene(_scene)
    {
        renderPass = VulkanRenderPass::Builder(vulkanDevice)
                .AddColorAttachment(VK_FORMAT_R32G32B32A32_SFLOAT, false)
                .AddColorAttachment(VK_FORMAT_R32G32B32A32_SFLOAT, false)
                .AddDepthAttachment()
                .Build();
        LoadPipelines();
    }

    void GBufferPass::Render(VkCommandBuffer commandBuffer, Camera& camera, Ref<VulkanFramebuffer> writeBuffer,
                             Ref<VulkanFramebuffer> readBuffer)
    {
        device->SetViewportAndScissor(writeBuffer->GetExtent(), commandBuffer);
        renderPass->Begin(commandBuffer, writeBuffer, writeBuffer->GetExtent());

        DrawCommandParams geometryDrawParams{};
        geometryDrawParams.commandBuffer = commandBuffer;
        geometryDrawParams.pipelineParams =
        {
            gbufferPipeline->GetPipeline(),
            gbufferPipeline->GetPipelineLayout()
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
                if (scene.meshes[i]->GetMaterial(meshlet).params.alphaTested) continue;

                geometryDrawParams.descriptorSets = {
                    scene.meshes[i]->GetMaterial(meshlet).descriptorSet,
                    ShaderCache::descriptorSets.transformDescriptorSet,
                };

                geometryDrawParams.meshlet = &meshlet;
                device->DrawMeshlet(geometryDrawParams);
            }
        }

        renderPass->End(commandBuffer);
    }

    void GBufferPass::LoadPipelines()
    {
        LOG_TRACE("Building geometry pipeline");
        PipelineConfig pipelineConfig; {
            pipelineConfig.vertexShaderPath = "gbuffer.vert";
            pipelineConfig.fragmentShaderPath = "gbuffer.frag";

            pipelineConfig.cullMode = PipelineCullMode::Back;
            pipelineConfig.polygonMode = PipelinePolygonMode::Fill;
            pipelineConfig.frontFace = PipelineFrontFace::Counter_clockwise;

            pipelineConfig.descriptorSetLayouts = {
                ShaderCache::descriptorSetLayouts.materialDescriptorSetLayout,
                ShaderCache::descriptorSetLayouts.transformDescriptorSetLayout,
            };

            pipelineConfig.colorAttachments = {
                ImageFormat::RGBA32_SFLOAT,
                ImageFormat::RGBA32_SFLOAT,
            };

            pipelineConfig.disableBlending = true;
            pipelineConfig.enableDepthTest = true;
            pipelineConfig.depthAttachment = ImageFormat::DEPTH24_STENCIL8;

            pipelineConfig.renderPass = renderPass;

            pipelineConfig.pushConstantData.shaderStageBits = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            pipelineConfig.pushConstantData.offset = 0;
            pipelineConfig.pushConstantData.size = sizeof(SimplePushConstantData);
        }
        gbufferPipeline = VulkanPipeline::Builder().Build(device, pipelineConfig);
    }
}
