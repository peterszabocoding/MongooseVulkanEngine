#include "renderer/vulkan/pass/skybox_pass.h"

#include <renderer/vulkan/vulkan_mesh.h>

#include "renderer/shader_cache.h"
#include "renderer/vulkan/vulkan_pipeline.h"
#include "resource/resource_manager.h"
#include "util/log.h"

namespace MongooseVK
{
    SkyboxPass::SkyboxPass(VulkanDevice* vulkanDevice, Scene& _scene, VkExtent2D _resolution): VulkanPass(vulkanDevice, _resolution),
        scene(_scene)
    {
        cubeMesh = ResourceManager::LoadMesh(device, "resources/models/cube.obj");
        LoadPipelines();
    }

    void SkyboxPass::Render(VkCommandBuffer commandBuffer, Camera* camera, FramebufferHandle writeBuffer)
    {
        const VulkanFramebuffer* framebuffer = device->GetFramebuffer(writeBuffer);

        device->SetViewportAndScissor(framebuffer->extent, commandBuffer);
        GetRenderPass()->Begin(commandBuffer, framebuffer->framebuffer, framebuffer->extent);

        DrawCommandParams skyboxDrawParams{};
        skyboxDrawParams.commandBuffer = commandBuffer;
        skyboxDrawParams.meshlet = &cubeMesh->GetMeshlets()[0];
        skyboxDrawParams.pipelineParams = {
            pipeline->pipeline,
            pipeline->pipelineLayout
        };
        skyboxDrawParams.descriptorSets = {
            ShaderCache::descriptorSets.cubemapDescriptorSet,
            ShaderCache::descriptorSets.cameraDescriptorSet
        };

        device->DrawMeshlet(skyboxDrawParams);

        GetRenderPass()->End(commandBuffer);
    }

    void SkyboxPass::LoadPipelines()
    {
        VulkanRenderPass::RenderPassConfig config;
        config.AddColorAttachment({
            .imageFormat = ImageFormat::RGBA16_SFLOAT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        });

        config.AddDepthAttachment({
            .depthFormat = ImageFormat::DEPTH24_STENCIL8,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        });

        renderPassHandle = device->CreateRenderPass(config);

        LOG_TRACE("Building skybox pipeline");
        PipelineCreate pipelineConfig;
        pipelineConfig.vertexShaderPath = "skybox.vert";
        pipelineConfig.fragmentShaderPath = "skybox.frag";

        pipelineConfig.cullMode = PipelineCullMode::Front;
        pipelineConfig.enableDepthTest = false;
        pipelineConfig.renderPass = GetRenderPass()->Get();
        pipelineConfig.colorAttachments = {ImageFormat::RGBA16_SFLOAT};
        pipelineConfig.depthAttachment = ImageFormat::DEPTH24_STENCIL8;
        pipelineConfig.descriptorSetLayouts = {
            ShaderCache::descriptorSetLayouts.cubemapDescriptorSetLayout,
            ShaderCache::descriptorSetLayouts.cameraDescriptorSetLayout,
        };

        pipelineHandle = VulkanPipelineBuilder().Build(device, pipelineConfig);
        pipeline = device->GetPipeline(pipelineHandle);
    }
}
