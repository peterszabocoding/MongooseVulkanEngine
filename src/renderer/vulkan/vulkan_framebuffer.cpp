#include "vulkan_framebuffer.h"

#include <array>

#include "vulkan_device.h"
#include "vulkan_utils.h"
#include "util/log.h"

namespace Raytracing
{
    VulkanFramebuffer::Builder& VulkanFramebuffer::Builder::AddAttachment(Ref<VulkanImage> imageAttachment)
    {
        attachments.push_back(imageAttachment);
        return *this;
    }

    VulkanFramebuffer::Builder& VulkanFramebuffer::Builder::SetRenderpass(Ref<VulkanRenderPass> renderPass)
    {
        this->renderPass = renderPass;
        return *this;
    }

    VulkanFramebuffer::Builder& VulkanFramebuffer::Builder::SetResolution(int width, int height)
    {
        this->width = width;
        this->height = height;
        return *this;
    }

    Ref<VulkanFramebuffer> VulkanFramebuffer::Builder::Build()
    {

        std::vector<VkImageView> imageViews(attachments.size());
        for (int i = 0; i < attachments.size(); i++)
        {
            imageViews[i] = attachments[i]->GetImageView()->Get();
        }

        VkFramebufferCreateInfo framebuffer_info{};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = renderPass->Get();
        framebuffer_info.attachmentCount = static_cast<uint32_t>(imageViews.size());
        framebuffer_info.pAttachments = imageViews.data();
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
