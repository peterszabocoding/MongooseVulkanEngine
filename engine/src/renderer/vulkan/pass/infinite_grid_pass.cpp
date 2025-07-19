#include "renderer/vulkan/pass/infinite_grid_pass.h"

#include "util/log.h"
#include "renderer/shader_cache.h"
#include "renderer/vulkan/vulkan_pipeline.h"
#include "renderer/vulkan/vulkan_mesh.h"

namespace MongooseVK
{
    InfiniteGridPass::InfiniteGridPass(VulkanDevice* vulkanDevice, VkExtent2D _resolution): VulkanPass(vulkanDevice, _resolution)
    {
        screenRect = CreateScope<VulkanMeshlet>(device, Primitives::GRID_VERTICES, Primitives::GRID_INDICES);
        LoadPipelines();
    }

    void InfiniteGridPass::Render(VkCommandBuffer commandBuffer, Camera* camera, FramebufferHandle writeBuffer)
    {
        VulkanFramebuffer* framebuffer = device->GetFramebuffer(writeBuffer);

        device->SetViewportAndScissor(framebuffer->extent, commandBuffer);
        GetRenderPass()->Begin(commandBuffer, framebuffer->framebuffer, framebuffer->extent);

        DrawCommandParams screenRectDrawParams{};
        screenRectDrawParams.commandBuffer = commandBuffer;
        screenRectDrawParams.meshlet = screenRect.get();
        screenRectDrawParams.pushConstantParams.data = &gridParams;
        screenRectDrawParams.pushConstantParams.size = sizeof(GridParams);

        screenRectDrawParams.pipelineParams = {
            pipeline->pipeline,
            pipeline->pipelineLayout
        };

        screenRectDrawParams.descriptorSets = {
            ShaderCache::descriptorSets.cameraDescriptorSet,
        };

        device->DrawMeshlet(screenRectDrawParams);
        GetRenderPass()->End(commandBuffer);
    }

    void InfiniteGridPass::LoadPipelines()
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

        LOG_TRACE("Building present pipeline");
        PipelineCreate pipelineConfig;
        pipelineConfig.vertexShaderPath = "infinite_grid.vert";
        pipelineConfig.fragmentShaderPath = "infinite_grid.frag";

        pipelineConfig.cullMode = PipelineCullMode::Unknown;
        pipelineConfig.polygonMode = PipelinePolygonMode::Fill;
        pipelineConfig.frontFace = PipelineFrontFace::Counter_clockwise;
        pipelineConfig.disableBlending = false;
        pipelineConfig.enableDepthTest = true;
        pipelineConfig.depthWriteEnable = false;
        pipelineConfig.renderPass = GetRenderPass()->Get();
        pipelineConfig.pushConstantData.shaderStageBits = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pipelineConfig.pushConstantData.size = sizeof(GridParams);

        pipelineConfig.descriptorSetLayouts = {
            ShaderCache::descriptorSetLayouts.cameraDescriptorSetLayout
        };

        pipelineConfig.colorAttachments = {
            ImageFormat::RGBA16_SFLOAT,
        };

        pipelineConfig.depthAttachment = ImageFormat::DEPTH24_STENCIL8;

        pipelineHandle = VulkanPipelineBuilder().Build(device, pipelineConfig);
        pipeline = device->GetPipeline(pipelineHandle);
    }
}
