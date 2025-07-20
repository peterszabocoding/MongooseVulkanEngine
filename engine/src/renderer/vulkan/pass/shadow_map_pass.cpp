#include "renderer/vulkan/pass/shadow_map_pass.h"

#include <renderer/vulkan/vulkan_mesh.h>

#include "renderer/shader_cache.h"
#include "util/log.h"

namespace MongooseVK
{
    ShadowMapPass::ShadowMapPass(VulkanDevice* vulkanDevice, Scene& _scene, VkExtent2D _resolution): VulkanPass(vulkanDevice, _resolution),
        scene(_scene)
    {
        LoadPipeline();
    }

    void ShadowMapPass::Render(VkCommandBuffer commandBuffer, Camera* camera, FramebufferHandle writeBuffer)
    {
        VulkanFramebuffer* framebuffer = device->GetFramebuffer(writeBuffer);

        device->SetViewportAndScissor(framebuffer->extent, commandBuffer);
        GetRenderPass()->Begin(commandBuffer, framebuffer->framebuffer, framebuffer->extent);

        DrawCommandParams geometryDrawParams{};
        geometryDrawParams.commandBuffer = commandBuffer;
        geometryDrawParams.pipelineParams = {
            pipeline->pipeline,
            pipeline->pipelineLayout
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

    void ShadowMapPass::LoadPipeline()
    {
        VulkanRenderPass::RenderPassConfig config;
        config.AddDepthAttachment({.depthFormat = ImageFormat::DEPTH32});

        renderPassHandle = device->CreateRenderPass(config);

        LOG_TRACE("Building directional shadow map pipeline");
        PipelineCreate pipelineConfig;
        pipelineConfig.vertexShaderPath = "depth_only.vert";
        pipelineConfig.fragmentShaderPath = "empty.frag";

        pipelineConfig.cullMode = PipelineCullMode::Front;
        pipelineConfig.polygonMode = PipelinePolygonMode::Fill;
        pipelineConfig.frontFace = PipelineFrontFace::Counter_clockwise;

        pipelineConfig.disableBlending = true;
        pipelineConfig.enableDepthTest = true;


        pipelineConfig.renderPass = GetRenderPass()->Get();

        pipelineConfig.pushConstantData.shaderStageBits = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pipelineConfig.pushConstantData.offset = 0;
        pipelineConfig.pushConstantData.size = sizeof(SimplePushConstantData);

        pipelineConfig.depthAttachment = ImageFormat::DEPTH32;

        pipelineHandle = VulkanPipelineBuilder().Build(device, pipelineConfig);
        pipeline = device->GetPipeline(pipelineHandle);
    }
}
