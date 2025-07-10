#include "renderer/vulkan/pass/present_pass.h"

#include <backends/imgui_impl_vulkan.h>

#include "imgui.h"
#include "util/log.h"
#include "renderer/shader_cache.h"
#include "renderer/vulkan/vulkan_mesh.h"

namespace MongooseVK
{
    PresentPass::PresentPass(VulkanDevice* vulkanDevice, VkExtent2D _resolution): VulkanPass(vulkanDevice, _resolution)
    {
        screenRect = CreateScope<VulkanMeshlet>(device, Primitives::RECTANGLE_VERTICES, Primitives::RECTANGLE_INDICES);
        LoadPipeline();
    }

    PresentPass::~PresentPass()
    {
        device->DestroyRenderPass(renderPassHandle);
    }

    void PresentPass::Render(VkCommandBuffer commandBuffer, Camera& camera, Ref<VulkanFramebuffer> writeBuffer,
                             Ref<VulkanFramebuffer> readBuffer)
    {
        device->SetViewportAndScissor(writeBuffer->GetExtent(), commandBuffer);
        GetRenderPass()->Begin(commandBuffer, writeBuffer, writeBuffer->GetExtent());

        DrawCommandParams screenRectDrawParams{};
        screenRectDrawParams.commandBuffer = commandBuffer;

        screenRectDrawParams.pipelineParams = {
            presentPipeline->GetPipeline(),
            presentPipeline->GetPipelineLayout()
        };

        screenRectDrawParams.descriptorSets = {
            ShaderCache::descriptorSets.presentDescriptorSet,
        };
        screenRectDrawParams.meshlet = screenRect.get();

        device->DrawMeshlet(screenRectDrawParams);
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

        GetRenderPass()->End(commandBuffer);
    }

    VulkanRenderPass* PresentPass::GetRenderPass()
    {
        return device->renderPassPool.Get(renderPassHandle.handle);
    }

    void PresentPass::LoadPipeline()
    {
        LOG_TRACE("Building present pipeline");
        VulkanRenderPass::RenderPassConfig config;
        config.AddColorAttachment({
            .imageFormat = VK_FORMAT_R8G8B8A8_UNORM,
            .isSwapchainAttachment = true
        });

        renderPassHandle = device->CreateRenderPass(config);

        PipelineConfig presentPipelineConfig;
        presentPipelineConfig.name = "present_pipeline";
        presentPipelineConfig.vertexShaderPath = "quad.vert";
        presentPipelineConfig.fragmentShaderPath = "quad.frag";

        presentPipelineConfig.cullMode = PipelineCullMode::Front;
        presentPipelineConfig.polygonMode = PipelinePolygonMode::Fill;
        presentPipelineConfig.frontFace = PipelineFrontFace::Counter_clockwise;

        presentPipelineConfig.descriptorSetLayouts = {
            ShaderCache::descriptorSetLayouts.presentDescriptorSetLayout
        };

        presentPipelineConfig.colorAttachments = {
            ImageFormat::RGBA8_UNORM,
        };

        presentPipelineConfig.disableBlending = true;
        presentPipelineConfig.enableDepthTest = false;

        presentPipelineConfig.renderPass = GetRenderPass()->Get();

        presentPipeline = VulkanPipeline::Builder().Build(device, presentPipelineConfig);
    }
}
