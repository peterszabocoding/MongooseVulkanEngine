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
    }

    void SkyboxPass::Render(VkCommandBuffer commandBuffer, Camera* camera, FramebufferHandle writeBuffer)
    {
        const VulkanFramebuffer* framebuffer = device->GetFramebuffer(framebufferHandles[0]);

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
            device->bindlessTextureDescriptorSet,
            device->materialDescriptorSet,
            passDescriptorSet
        };

        device->DrawMeshlet(skyboxDrawParams);

        GetRenderPass()->End(commandBuffer);
    }

    void SkyboxPass::LoadPipeline()
    {
        LOG_TRACE("Building skybox pipeline");
        pipelineConfig.vertexShaderPath = "skybox.vert";
        pipelineConfig.fragmentShaderPath = "skybox.frag";

        pipelineConfig.cullMode = PipelineCullMode::Front;
        pipelineConfig.enableDepthTest = false;
        pipelineConfig.renderPass = GetRenderPass()->Get();
        pipelineConfig.descriptorSetLayouts = {
            device->bindlessTexturesDescriptorSetLayoutHandle,
            device->materialsDescriptorSetLayoutHandle,
            passDescriptorSetLayoutHandle
        };

        pipelineHandle = VulkanPipelineBuilder().Build(device, pipelineConfig);
        pipeline = device->GetPipeline(pipelineHandle);
    }
}
