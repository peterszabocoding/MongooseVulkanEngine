#include "vulkan_framebuffer.h"

#include <array>

#include "vulkan_device.h"
#include "vulkan_utils.h"
#include "util/log.h"

namespace Raytracing
{
    VulkanFramebuffer::Builder& VulkanFramebuffer::Builder::AddAttachment(VkImageView imageAttachment)
    {
        attachments.push_back(imageAttachment);
        return *this;
    }

    VulkanFramebuffer::Builder& VulkanFramebuffer::Builder::SetRenderpass(Ref<VulkanRenderPass> _renderPass)
    {
        renderPass = _renderPass;
        return *this;
    }

    VulkanFramebuffer::Builder& VulkanFramebuffer::Builder::SetResolution(int _width, int _height)
    {
        width = _width;
        height = _height;
        return *this;
    }

    Ref<VulkanFramebuffer> VulkanFramebuffer::Builder::Build()
    {
        VkFramebufferCreateInfo framebuffer_info{};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = renderPass->Get();
        framebuffer_info.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebuffer_info.pAttachments = attachments.data();
        framebuffer_info.width = width;
        framebuffer_info.height = height;
        framebuffer_info.layers = 1;

        VkFramebuffer framebuffer = VK_NULL_HANDLE;
        VK_CHECK_MSG(vkCreateFramebuffer(device->GetDevice(), &framebuffer_info, nullptr, &framebuffer),
                     "Failed to create framebuffer.");

        return CreateRef<VulkanFramebuffer>(device, framebuffer);
    }

    VulkanFramebuffer::~VulkanFramebuffer()
    {
        LOG_INFO("Destroy framebuffer");
        vkDestroyFramebuffer(device->GetDevice(), framebuffer, nullptr);
    }
}
