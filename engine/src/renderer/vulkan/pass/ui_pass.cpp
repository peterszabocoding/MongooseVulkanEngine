#include "renderer\vulkan\pass\ui_pass.h"

#include <backends/imgui_impl_vulkan.h>
#include <renderer/vulkan/vulkan_framebuffer.h>

namespace MongooseVK
{
    UiPass::UiPass(VulkanDevice* vulkanDevice, VkExtent2D _resolution): FrameGraphRenderPass(vulkanDevice, _resolution)
    {}

    void UiPass::Render(VkCommandBuffer commandBuffer, Scene* scene)
    {
        const VulkanFramebuffer* framebuffer = device->GetFramebuffer(framebufferHandles[0]);
        device->SetViewportAndScissor(framebuffer->extent, commandBuffer);

        GetRenderPass()->Begin(commandBuffer, framebuffer->framebuffer, framebuffer->extent);
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
        GetRenderPass()->End(commandBuffer);
    }

    void UiPass::LoadPipeline() {}
}
