#include "shadow_map_pass.h"

#include "renderer/shader_cache.h"
#include "util/log.h"

namespace Raytracing
{
    ShadowMapPass::ShadowMapPass(VulkanDevice* vulkanDevice, Scene& _scene): VulkanPass(vulkanDevice), scene(_scene)
    {
        renderPass = VulkanRenderPass::Builder(vulkanDevice)
                .AddDepthAttachment(VK_FORMAT_D32_SFLOAT)
                .Build();
        LoadPipelines();
    }

    void ShadowMapPass::Render(VkCommandBuffer commandBuffer, Camera& camera, uint32_t imageIndex, Ref<VulkanFramebuffer> writeBuffer,
                               Ref<VulkanFramebuffer> readBuffer)
    {
        constexpr VkExtent2D extent = {1024, 1024};

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
            //pushConstantData.transform = scene.directionalLight.GetProjection() * scene.directionalLight.GetView();
            pushConstantData.transform = scene.directionalLight.cascades[2].viewProjMatrix;
            pushConstantData.modelMatrix = scene.transforms[i].GetTransform();

            geometryDrawParams.pushConstantParams = {
                &pushConstantData,
                sizeof(SimplePushConstantData)
            };

            for (auto& meshlet: scene.meshes[i]->GetMeshlets())
            {
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
            dirShadowMapPipelineConfig.vertexShaderPath = "depth_only.vert";
            dirShadowMapPipelineConfig.fragmentShaderPath = "empty.frag";

            dirShadowMapPipelineConfig.cullMode = PipelineCullMode::Front;
            dirShadowMapPipelineConfig.polygonMode = PipelinePolygonMode::Fill;
            dirShadowMapPipelineConfig.frontFace = PipelineFrontFace::Counter_clockwise;

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
