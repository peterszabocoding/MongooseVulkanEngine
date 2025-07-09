#include "renderer/vulkan/pass/skybox_pass.h"

#include "renderer/shader_cache.h"
#include "renderer/vulkan/vulkan_pipeline.h"
#include "resource/resource_manager.h"
#include "util/log.h"

namespace Raytracing
{
    SkyboxPass::SkyboxPass(VulkanDevice* vulkanDevice, Scene& _scene, VkExtent2D _resolution): VulkanPass(vulkanDevice, _resolution), scene(_scene)
    {
        VulkanRenderPass::ColorAttachment colorAttachment;
        colorAttachment.imageFormat = VK_FORMAT_R16G16B16A16_UNORM;

        renderPass = VulkanRenderPass::Builder(vulkanDevice)
                .AddColorAttachment(colorAttachment)
                .Build();

        cubeMesh = ResourceManager::LoadMesh(device, "resources/models/cube.obj");
        LoadPipelines();
    }

    void SkyboxPass::Render(VkCommandBuffer commandBuffer, Camera& camera, Ref<VulkanFramebuffer> writeBuffer,
                            Ref<VulkanFramebuffer> readBuffer)
    {
        device->SetViewportAndScissor(writeBuffer->GetExtent(), commandBuffer);
        renderPass->Begin(commandBuffer, writeBuffer, writeBuffer->GetExtent());

        DrawCommandParams skyboxDrawParams{};
        skyboxDrawParams.commandBuffer = commandBuffer;
        skyboxDrawParams.meshlet = &cubeMesh->GetMeshlets()[0];

        skyboxDrawParams.pipelineParams = {
            skyboxPipeline->GetPipeline(),
            skyboxPipeline->GetPipelineLayout()
        };

        skyboxDrawParams.descriptorSets = {
            scene.skybox->descriptorSet,
            ShaderCache::descriptorSets.transformDescriptorSet
        };

        device->DrawMeshlet(skyboxDrawParams);

        renderPass->End(commandBuffer);
    }

    void SkyboxPass::LoadPipelines()
    {
        LOG_TRACE("Building skybox pipeline");
        PipelineConfig skyboxPipelineConfig; {
            skyboxPipelineConfig.vertexShaderPath = "skybox.vert";
            skyboxPipelineConfig.fragmentShaderPath = "skybox.frag";

            skyboxPipelineConfig.cullMode = PipelineCullMode::Front;
            skyboxPipelineConfig.polygonMode = PipelinePolygonMode::Fill;
            skyboxPipelineConfig.frontFace = PipelineFrontFace::Counter_clockwise;

            skyboxPipelineConfig.descriptorSetLayouts = {
                ShaderCache::descriptorSetLayouts.cubemapDescriptorSetLayout,
                ShaderCache::descriptorSetLayouts.transformDescriptorSetLayout,
            };

            skyboxPipelineConfig.colorAttachments = {
                ImageFormat::RGBA16_SFLOAT,
            };

            skyboxPipelineConfig.disableBlending = true;
            skyboxPipelineConfig.enableDepthTest = false;

            skyboxPipelineConfig.renderPass = renderPass;
        }
        skyboxPipeline = VulkanPipeline::Builder().Build(device, skyboxPipelineConfig);
    }
}
