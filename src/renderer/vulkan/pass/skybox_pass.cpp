//
// Created by peter on 2025. 05. 24..
//

#include "skybox_pass.h"

#include "renderer/shader_cache.h"
#include "renderer/vulkan/vulkan_pipeline.h"
#include "resource/resource_manager.h"
#include "util/log.h"

namespace Raytracing {
    SkyboxPass::SkyboxPass(VulkanDevice* vulkanDevice, Scene& _scene): VulkanPass(vulkanDevice), scene(_scene)
    {
        renderPass = VulkanRenderPass::Builder(vulkanDevice)
                .AddColorAttachment(VK_FORMAT_R8G8B8A8_UNORM, false)
                .Build();

        cubeMesh = ResourceManager::LoadMesh(device, "resources/models/cube.obj");
        LoadPipelines();
    }

    void SkyboxPass::Render(VkCommandBuffer commandBuffer, uint32_t imageIndex, Ref<VulkanFramebuffer> writeBuffer,
        Ref<VulkanFramebuffer> readBuffer)
    {
        VkExtent2D extent = {passWidth, passHeight};

        device->SetViewportAndScissor(extent, commandBuffer);
        renderPass->Begin(commandBuffer, writeBuffer, extent);

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
            skyboxPipelineConfig.vertexShaderPath = "shader\\glsl\\skybox.vert";
            skyboxPipelineConfig.fragmentShaderPath = "shader\\glsl\\skybox.frag";

            skyboxPipelineConfig.cullMode = PipelineCullMode::Front;
            skyboxPipelineConfig.polygonMode = PipelinePolygonMode::Fill;
            skyboxPipelineConfig.frontFace = PipelineFrontFace::Counter_clockwise;

            skyboxPipelineConfig.descriptorSetLayouts = {
                ShaderCache::descriptorSetLayouts.cubemapDescriptorSetLayout,
                ShaderCache::descriptorSetLayouts.transformDescriptorSetLayout,
            };

            skyboxPipelineConfig.colorAttachments = {
                ImageFormat::RGBA8_UNORM,
            };

            skyboxPipelineConfig.disableBlending = true;
            skyboxPipelineConfig.enableDepthTest = false;

            skyboxPipelineConfig.renderPass = renderPass;
        }
        skyboxPipeline = VulkanPipeline::Builder().Build(device, skyboxPipelineConfig);
    }
}
