#include "renderer/vulkan/vulkan_renderpass.h"
#include "renderer/vulkan/vulkan_device.h"

#include <array>

#include "renderer/vulkan/vulkan_framebuffer.h"
#include "renderer/vulkan/vulkan_utils.h"

namespace MongooseVK
{
    void VulkanRenderPass::Begin(const VkCommandBuffer commandBuffer, const VkFramebuffer framebuffer, const VkExtent2D extent)
    {
        std::vector<VkClearValue> clearValues;

        for (size_t i = 0; i < config.numColorAttachments; i++)
        {
            const ColorAttachment colorAttachment = config.colorAttachments[i];
            clearValues.push_back({
                .color = {
                    {
                        colorAttachment.clearColor.x,
                        colorAttachment.clearColor.y,
                        colorAttachment.clearColor.z,
                        colorAttachment.clearColor.w
                    }
                }
            });
        }

        if (config.depthAttachment.has_value())
        {
            clearValues.push_back({.depthStencil = {1.0f, 0}});
        }


        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = framebuffer;
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = extent;
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    void VulkanRenderPass::End(const VkCommandBuffer commandBuffer)
    {
        vkCmdEndRenderPass(commandBuffer);
    }
}
