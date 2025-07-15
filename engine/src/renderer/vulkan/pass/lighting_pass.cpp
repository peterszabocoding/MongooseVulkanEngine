#include "renderer/vulkan/pass/lighting_pass.h"

#include "renderer/shader_cache.h"
#include "resource/resource_manager.h"
#include "util/log.h"

namespace MongooseVK
{
    LightingPass::LightingPass(VulkanDevice* vulkanDevice, Scene& _scene, VkExtent2D _resolution): VulkanPass(vulkanDevice, _resolution),
        scene(_scene)
    {
        LoadPipelines();
    }

    void LightingPass::Render(VkCommandBuffer commandBuffer, Camera* camera, Ref<VulkanFramebuffer> writeBuffer,
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
            pushConstantData.transform = camera->GetProjection() * camera->GetView() * pushConstantData.modelMatrix;

            geometryDrawParams.pushConstantParams = {
                &pushConstantData,
                sizeof(SimplePushConstantData)
            };

            for (auto& meshlet: scene.meshes[i]->GetMeshlets())
            {
                geometryDrawParams.descriptorSets = {
                    device->bindlessTextureDescriptorSet,
                    scene.meshes[i]->GetMaterial(meshlet).descriptorSet,
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

    void LightingPass::LoadPipelines()
    {
        VulkanRenderPass::RenderPassConfig config;
        config.AddColorAttachment({
            .imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD
        });
        config.AddDepthAttachment({
            .depthFormat = VK_FORMAT_D24_UNORM_S8_UINT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD
        });

        renderPassHandle = device->CreateRenderPass(config);

        LOG_TRACE("Building lighting pipeline");
        PipelineConfig pipelineConfig;
        pipelineConfig.vertexShaderPath = "base-pass.vert";
        pipelineConfig.fragmentShaderPath = "lighting-pass.frag";

        pipelineConfig.cullMode = PipelineCullMode::Back;
        pipelineConfig.polygonMode = PipelinePolygonMode::Fill;
        pipelineConfig.frontFace = PipelineFrontFace::Counter_clockwise;

        pipelineConfig.descriptorSetLayouts = {
            device->bindlessDescriptorSetLayout,
            ShaderCache::descriptorSetLayouts.materialDescriptorSetLayout,
            ShaderCache::descriptorSetLayouts.cameraDescriptorSetLayout,
            ShaderCache::descriptorSetLayouts.lightsDescriptorSetLayout,
            ShaderCache::descriptorSetLayouts.directionalShadowMapDescriptorSetLayout,
            ShaderCache::descriptorSetLayouts.irradianceDescriptorSetLayout,
            ShaderCache::descriptorSetLayouts.postProcessingDescriptorSetLayout,
            ShaderCache::descriptorSetLayouts.reflectionDescriptorSetLayout,
            ShaderCache::descriptorSetLayouts.brdfLutDescriptorSetLayout,
        };

        pipelineConfig.disableBlending = false;
        pipelineConfig.enableDepthTest = true;
        pipelineConfig.depthWriteEnable = true;

        pipelineConfig.renderPass = GetRenderPass()->Get();

        pipelineConfig.pushConstantData.shaderStageBits = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pipelineConfig.pushConstantData.offset = 0;
        pipelineConfig.pushConstantData.size = sizeof(SimplePushConstantData);

        pipelineConfig.colorAttachments = {
            ImageFormat::RGBA16_SFLOAT,
        };

        pipelineConfig.depthAttachment = ImageFormat::DEPTH24_STENCIL8;

        geometryPipeline = VulkanPipeline::Builder().Build(device, pipelineConfig);
    }
}
