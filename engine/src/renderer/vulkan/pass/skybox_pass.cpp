#include "renderer/vulkan/pass/skybox_pass.h"

#include <renderer/vulkan/vulkan_framebuffer.h>
#include <renderer/vulkan/vulkan_mesh.h>

#include "renderer/shader_cache.h"
#include "renderer/vulkan/vulkan_pipeline.h"
#include "resource/resource_manager.h"
#include "util/log.h"

namespace MongooseVK
{
    SkyboxPass::SkyboxPass(VulkanDevice* vulkanDevice, VkExtent2D _resolution): FrameGraphRenderPass(vulkanDevice, _resolution)
    {
        cubeMesh = ResourceManager::LoadMesh(device, "resources/models/cube.obj");
    }

    void SkyboxPass::Render(VkCommandBuffer commandBuffer, Scene* scene)
    {
        const VulkanFramebuffer* framebuffer = device->GetFramebuffer(framebufferHandles[0]);

        device->SetViewportAndScissor(framebuffer->extent, commandBuffer);
        GetRenderPass()->Begin(commandBuffer, framebuffer->framebuffer, framebuffer->extent);

        DrawCommandParams drawCommandParams{};
        drawCommandParams.commandBuffer = commandBuffer;
        drawCommandParams.meshlet = &cubeMesh->GetMeshlets()[0];
        drawCommandParams.pipelineParams = {
            pipeline->pipeline,
            pipeline->pipelineLayout
        };
        drawCommandParams.descriptorSets = {
            device->bindlessTextureDescriptorSet,
            device->materialDescriptorSet,
            passDescriptorSet
        };

        SkyboxPushConstantData pushConstantData;
        pushConstantData.skyboxTextureIndex = scene->skyboxTexture.handle;

        drawCommandParams.pushConstantParams = {
            &pushConstantData,
            sizeof(SkyboxPushConstantData)
        };

        device->DrawMeshlet(drawCommandParams);

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

        pipelineConfig.pushConstantData.shaderStageBits = VK_SHADER_STAGE_FRAGMENT_BIT;
        pipelineConfig.pushConstantData.offset = 0;
        pipelineConfig.pushConstantData.size = sizeof(SkyboxPushConstantData);

        pipelineHandle = VulkanPipelineBuilder().Build(device, pipelineConfig);
        pipeline = device->GetPipeline(pipelineHandle);
    }
}
