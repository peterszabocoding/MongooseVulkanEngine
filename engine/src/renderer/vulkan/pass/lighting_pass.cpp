#include "renderer/vulkan/pass/lighting_pass.h"

#include <renderer/vulkan/vulkan_mesh.h>

#include "renderer/shader_cache.h"
#include "resource/resource_manager.h"
#include "util/log.h"

namespace MongooseVK
{
    LightingPass::LightingPass(VulkanDevice* vulkanDevice, Scene& _scene, VkExtent2D _resolution): VulkanPass(vulkanDevice, _resolution),
        scene(_scene)
    {
        LoadPipeline();
    }

    void LightingPass::Render(VkCommandBuffer commandBuffer, Camera* camera, FramebufferHandle writeBuffer)
    {
        VulkanFramebuffer* framebuffer = device->GetFramebuffer(writeBuffer);

        device->SetViewportAndScissor(framebuffer->extent, commandBuffer);
        GetRenderPass()->Begin(commandBuffer, framebuffer->framebuffer, framebuffer->extent);

        DrawCommandParams geometryDrawParams{};
        geometryDrawParams.commandBuffer = commandBuffer;
        geometryDrawParams.pipelineParams =
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
                pushConstantData.transform = camera->GetProjection() * camera->GetView() * pushConstantData.modelMatrix;
                pushConstantData.materialIndex = material->index;

                geometryDrawParams.pushConstantParams = {
                    &pushConstantData,
                    sizeof(SimplePushConstantData)
                };

                geometryDrawParams.descriptorSets = {
                    device->bindlessTextureDescriptorSet,
                    device->materialDescriptorSet,
                    ShaderCache::descriptorSets.cameraDescriptorSet,
                    ShaderCache::descriptorSets.lightsDescriptorSet,
                    ShaderCache::descriptorSets.directionalShadownMapDescriptorSet,
                    ShaderCache::descriptorSets.irradianceDescriptorSet,
                    ShaderCache::descriptorSets.postProcessingDescriptorSet,
                    ShaderCache::descriptorSets.reflectionDescriptorSet,
                    ShaderCache::descriptorSets.brdfLutDescriptorSet,
                };

                geometryDrawParams.meshlet = &meshlet;
                device->DrawMeshlet(geometryDrawParams);
            }
        }

        GetRenderPass()->End(commandBuffer);
    }

    void LightingPass::LoadPipeline()
    {
        VulkanRenderPass::RenderPassConfig config;
        config.AddColorAttachment({
            .imageFormat = ImageFormat::RGBA16_SFLOAT,
            .loadOp = RenderPassOperation::LoadOp::Load
        });
        config.AddDepthAttachment({
            .depthFormat = ImageFormat::DEPTH24_STENCIL8,
            .loadOp = RenderPassOperation::LoadOp::Load
        });

        renderPassHandle = device->CreateRenderPass(config);

        LOG_TRACE("Building lighting pipeline");
        PipelineCreate pipelineConfig;
        pipelineConfig.vertexShaderPath = "base-pass.vert";
        pipelineConfig.fragmentShaderPath = "lighting-pass.frag";

        pipelineConfig.cullMode = PipelineCullMode::Back;

        pipelineConfig.descriptorSetLayouts = {
            device->bindlessTexturesDescriptorSetLayoutHandle,
            device->materialsDescriptorSetLayoutHandle,
            ShaderCache::descriptorSetLayouts.cameraDescriptorSetLayout,
            ShaderCache::descriptorSetLayouts.lightsDescriptorSetLayout,
            ShaderCache::descriptorSetLayouts.directionalShadowMapDescriptorSetLayout,
            ShaderCache::descriptorSetLayouts.irradianceDescriptorSetLayout,
            ShaderCache::descriptorSetLayouts.postProcessingDescriptorSetLayout,
            ShaderCache::descriptorSetLayouts.reflectionDescriptorSetLayout,
            ShaderCache::descriptorSetLayouts.brdfLutDescriptorSetLayout,
        };

        pipelineConfig.disableBlending = false;

        pipelineConfig.renderPass = GetRenderPass()->Get();

        pipelineConfig.pushConstantData.shaderStageBits = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pipelineConfig.pushConstantData.offset = 0;
        pipelineConfig.pushConstantData.size = sizeof(SimplePushConstantData);

        pipelineConfig.colorAttachments = {
            ImageFormat::RGBA16_SFLOAT,
        };

        pipelineConfig.depthAttachment = ImageFormat::DEPTH24_STENCIL8;

        pipelineHandle = VulkanPipelineBuilder().Build(device, pipelineConfig);
        pipeline = device->GetPipeline(pipelineHandle);
    }
}
