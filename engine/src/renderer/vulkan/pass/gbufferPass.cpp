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
        scene(_scene)
    {
        LoadPipelines();
    }

    void GBufferPass::Render(VkCommandBuffer commandBuffer, Camera* camera, FramebufferHandle writeBuffer)
    {
        VulkanFramebuffer* framebuffer = device->GetFramebuffer(writeBuffer);

        device->SetViewportAndScissor(resolution, commandBuffer);
        GetRenderPass()->Begin(commandBuffer, framebuffer->framebuffer, resolution);

        DrawCommandParams geometryDrawParams{};
        geometryDrawParams.commandBuffer = commandBuffer;
        geometryDrawParams.pipelineParams =
        {
            pipeline->pipeline,
            pipeline->pipelineLayout
        };

        for (size_t i = 0; i < scene.meshes.size(); i++)
        {
            SimplePushConstantData pushConstantData;
            pushConstantData.modelMatrix = scene.transforms[i].GetTransform();
            pushConstantData.transform = camera->GetProjection() * camera->GetView() * pushConstantData.modelMatrix;

            geometryDrawParams.pushConstantParams = {
                &pushConstantData,
                sizeof(SimplePushConstantData)
            };

            for (auto& meshlet: scene.meshes[i]->GetMeshlets())
            {
                if (scene.meshes[i]->GetMaterial(meshlet).params.alphaTested) continue;

                geometryDrawParams.descriptorSets = {
                    device->bindlessTextureDescriptorSet,
                    scene.meshes[i]->GetMaterial(meshlet).descriptorSet,
                    ShaderCache::descriptorSets.cameraDescriptorSet,
                };

                geometryDrawParams.meshlet = &meshlet;
                device->DrawMeshlet(geometryDrawParams);
            }
        }

        GetRenderPass()->End(commandBuffer);
    }

    void GBufferPass::Resize(VkExtent2D _resolution)
    {
        VulkanPass::Resize(_resolution);
    }

    void GBufferPass::LoadPipelines()
    {
        LOG_TRACE("Building gbuffer pipeline");

        VulkanRenderPass::RenderPassConfig config;
        config.AddColorAttachment({.imageFormat = ImageFormat::RGBA32_SFLOAT});
        config.AddColorAttachment({.imageFormat = ImageFormat::RGBA32_SFLOAT});
        config.AddDepthAttachment({.depthFormat = ImageFormat::DEPTH24_STENCIL8});

        renderPassHandle = device->CreateRenderPass(config);

        constexpr PipelinePushConstantData pushConstantData = {
            .shaderStageBits = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            .offset = 0,
            .size = sizeof(SimplePushConstantData),
        };

        PipelineCreate pipelineConfig;
        pipelineConfig.vertexShaderPath = "gbuffer.vert";
        pipelineConfig.fragmentShaderPath = "gbuffer.frag";

        pipelineConfig.descriptorSetLayouts = {
            device->bindlessDescriptorSetLayoutHandle,
            ShaderCache::descriptorSetLayouts.materialDescriptorSetLayout,
            ShaderCache::descriptorSetLayouts.cameraDescriptorSetLayout,
        };

        pipelineConfig.renderPass = GetRenderPass()->Get();
        pipelineConfig.pushConstantData = pushConstantData;
        pipelineConfig.colorAttachments = {
            ImageFormat::RGBA32_SFLOAT,
            ImageFormat::RGBA32_SFLOAT,
        };
        pipelineConfig.depthAttachment = ImageFormat::DEPTH24_STENCIL8;

        pipelineHandle = VulkanPipelineBuilder().Build(device, pipelineConfig);
        pipeline = device->GetPipeline(pipelineHandle);
    }
}
