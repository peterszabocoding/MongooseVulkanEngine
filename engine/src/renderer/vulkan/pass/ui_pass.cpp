#include "renderer\vulkan\pass\ui_pass.h"

#include <backends/imgui_impl_vulkan.h>

namespace MongooseVK
{
    UiPass::UiPass(VulkanDevice* vulkanDevice, VkExtent2D _resolution): VulkanPass(vulkanDevice, _resolution)
    {}

    void UiPass::Render(VkCommandBuffer commandBuffer)
    {
        const VulkanFramebuffer* framebuffer = device->GetFramebuffer(framebufferHandles[0]);
        device->SetViewportAndScissor(framebuffer->extent, commandBuffer);

        GetRenderPass()->Begin(commandBuffer, framebuffer->framebuffer, framebuffer->extent);
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
        GetRenderPass()->End(commandBuffer);
    }

    void UiPass::LoadPipeline() {}
}
