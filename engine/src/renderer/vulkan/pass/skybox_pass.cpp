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
            skyboxPipeline->pipeline,
            skyboxPipeline->pipelineLayout
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
            .imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        });

        config.AddDepthAttachment({
            .depthFormat = VK_FORMAT_D24_UNORM_S8_UINT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        });

        renderPassHandle = device->CreateRenderPass(config);

        LOG_TRACE("Building skybox pipeline");
        PipelineCreate skyboxPipelineConfig;
        skyboxPipelineConfig.vertexShaderPath = "skybox.vert";
        skyboxPipelineConfig.fragmentShaderPath = "skybox.frag";

        skyboxPipelineConfig.cullMode = PipelineCullMode::Front;
        skyboxPipelineConfig.enableDepthTest = false;
        skyboxPipelineConfig.renderPass = GetRenderPass()->Get();
        skyboxPipelineConfig.colorAttachments = {ImageFormat::RGBA16_SFLOAT,};
        skyboxPipelineConfig.depthAttachment = ImageFormat::DEPTH24_STENCIL8;
        skyboxPipelineConfig.descriptorSetLayouts = {
            ShaderCache::descriptorSetLayouts.cubemapDescriptorSetLayout,
            ShaderCache::descriptorSetLayouts.cameraDescriptorSetLayout,
        };

        skyboxPipeline = VulkanPipelineBuilder().Build(device, skyboxPipelineConfig);
    }
}
