#include "renderer/vulkan/pass/render_pass.h"

#include "renderer/shader_cache.h"
#include "resource/resource_manager.h"
#include "util/log.h"

namespace MongooseVK
{
    RenderPass::RenderPass(VulkanDevice* vulkanDevice, Scene& _scene, VkExtent2D _resolution): VulkanPass(vulkanDevice, _resolution),
        scene(_scene)
    {
        VulkanRenderPass::RenderPassConfig config;
        config.AddColorAttachment({
            .imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD
        });
        config.AddDepthAttachment({.depthFormat = VK_FORMAT_D24_UNORM_S8_UINT});

        renderPassHandle = device->CreateRenderPass(config);

        LoadPipelines();
    }

    RenderPass::~RenderPass()
    {
        device->DestroyRenderPass(renderPassHandle);
    }

    void RenderPass::Render(VkCommandBuffer commandBuffer, Camera& camera, Ref<VulkanFramebuffer> writeBuffer,
                            Ref<VulkanFramebuffer> readBuffer)
    {
        device->SetViewportAndScissor(writeBuffer->GetExtent(), commandBuffer);
        GetRenderPass()->Begin(commandBuffer, writeBuffer, writeBuffer->GetExtent());

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
                    device->bindlessTextureDescriptorSet,
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

        GetRenderPass()->End(commandBuffer);
    }

    VulkanRenderPass* RenderPass::GetRenderPass()
    {
        return device->renderPassPool.Get(renderPassHandle.handle);
    }

    void RenderPass::LoadPipelines()
    {
        LOG_TRACE("Building lighting pipeline");
        PipelineConfig pipelineConfig; {
            pipelineConfig.vertexShaderPath = "base-pass.vert";
            pipelineConfig.fragmentShaderPath = "lighting-pass.frag";

            pipelineConfig.cullMode = PipelineCullMode::Back;
            pipelineConfig.polygonMode = PipelinePolygonMode::Fill;
            pipelineConfig.frontFace = PipelineFrontFace::Counter_clockwise;

            pipelineConfig.descriptorSetLayouts = {
                device->bindlessDescriptorSetLayout,
                ShaderCache::descriptorSetLayouts.materialDescriptorSetLayout,
                ShaderCache::descriptorSetLayouts.transformDescriptorSetLayout,
                ShaderCache::descriptorSetLayouts.lightsDescriptorSetLayout,
                ShaderCache::descriptorSetLayouts.irradianceDescriptorSetLayout,
                ShaderCache::descriptorSetLayouts.postProcessingDescriptorSetLayout,
                ShaderCache::descriptorSetLayouts.reflectionDescriptorSetLayout,
            };

            pipelineConfig.colorAttachments = {
                ImageFormat::RGBA16_SFLOAT,
            };

            pipelineConfig.disableBlending = true;
            pipelineConfig.enableDepthTest = true;
            pipelineConfig.depthAttachment = ImageFormat::DEPTH24_STENCIL8;

            pipelineConfig.renderPass = GetRenderPass()->Get();

            pipelineConfig.pushConstantData.shaderStageBits = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            pipelineConfig.pushConstantData.offset = 0;
            pipelineConfig.pushConstantData.size = sizeof(SimplePushConstantData);
        }
        geometryPipeline = VulkanPipeline::Builder().Build(device, pipelineConfig);
    }
}
