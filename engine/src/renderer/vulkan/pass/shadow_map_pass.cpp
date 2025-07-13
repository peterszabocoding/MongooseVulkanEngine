#include "renderer/vulkan/pass/shadow_map_pass.h"

#include "renderer/shader_cache.h"
#include "util/log.h"

namespace MongooseVK
{
    ShadowMapPass::ShadowMapPass(VulkanDevice* vulkanDevice, Scene& _scene, VkExtent2D _resolution): VulkanPass(vulkanDevice, _resolution),
        scene(_scene)
    {
        LoadPipelines();
    }

    void ShadowMapPass::Render(VkCommandBuffer commandBuffer, Camera* camera, Ref<VulkanFramebuffer> writeBuffer,
                               Ref<VulkanFramebuffer> readBuffer)
    {
        device->SetViewportAndScissor(writeBuffer->GetExtent(), commandBuffer);
        GetRenderPass()->Begin(commandBuffer, writeBuffer, writeBuffer->GetExtent());

        DrawCommandParams geometryDrawParams{};
        geometryDrawParams.commandBuffer = commandBuffer;
        geometryDrawParams.pipelineParams = {
            directionalShadowMapPipeline->GetPipeline(),
            directionalShadowMapPipeline->GetPipelineLayout()
        };

        SimplePushConstantData pushConstantData;

        pushConstantData.transform = scene.directionalLight.cascades[cascadeIndex].viewProjMatrix;

        for (size_t i = 0; i < scene.meshes.size(); i++)
        {
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

        GetRenderPass()->End(commandBuffer);
    }

    void ShadowMapPass::LoadPipelines()
    {
        VulkanRenderPass::RenderPassConfig config;
        config.AddDepthAttachment({.depthFormat = VK_FORMAT_D32_SFLOAT});

        renderPassHandle = device->CreateRenderPass(config);

        LOG_TRACE("Building directional shadow map pipeline");
        PipelineConfig dirShadowMapPipelineConfig;
        dirShadowMapPipelineConfig.vertexShaderPath = "depth_only.vert";
        dirShadowMapPipelineConfig.fragmentShaderPath = "empty.frag";

        dirShadowMapPipelineConfig.cullMode = PipelineCullMode::Front;
        dirShadowMapPipelineConfig.polygonMode = PipelinePolygonMode::Fill;
        dirShadowMapPipelineConfig.frontFace = PipelineFrontFace::Counter_clockwise;

        dirShadowMapPipelineConfig.disableBlending = true;
        dirShadowMapPipelineConfig.enableDepthTest = true;


        dirShadowMapPipelineConfig.renderPass = GetRenderPass()->Get();

        dirShadowMapPipelineConfig.pushConstantData.shaderStageBits = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        dirShadowMapPipelineConfig.pushConstantData.offset = 0;
        dirShadowMapPipelineConfig.pushConstantData.size = sizeof(SimplePushConstantData);

        dirShadowMapPipelineConfig.depthAttachment = ImageFormat::DEPTH32;

        directionalShadowMapPipeline = VulkanPipeline::Builder().Build(device, dirShadowMapPipelineConfig);
    }
}
