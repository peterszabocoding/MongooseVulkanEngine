#include "vulkan_framebuffer.h"

#include <array>

#include "vulkan_device.h"
#include "vulkan_utils.h"
#include "util/log.h"

namespace Raytracing
{
    namespace ImageUtils
    {
        VkImageAspectFlagBits GetAspectFlagFromFormat(ImageFormat format)
        {
            if (format == ImageFormat::DEPTH24_STENCIL8 || format == ImageFormat::DEPTH32)
                return VK_IMAGE_ASPECT_DEPTH_BIT;

            return VK_IMAGE_ASPECT_COLOR_BIT;
        }

        VkImageUsageFlags GetUsageFromFormat(ImageFormat format)
        {
            if (format == ImageFormat::DEPTH24_STENCIL8 || format == ImageFormat::DEPTH32)
                return VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

            return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
        }
    }

    VulkanFramebuffer::Builder& VulkanFramebuffer::Builder::AddAttachment(VkImageView imageAttachment)
    {
        FramebufferAttachment attachment;
        attachment.allocatedImage.image = nullptr;
        attachment.allocatedImage.allocation = nullptr;
        attachment.imageView = imageAttachment;
        attachment.format = VK_FORMAT_UNDEFINED;

        attachments.push_back(attachment);
        return *this;
    }

    VulkanFramebuffer::Builder& VulkanFramebuffer::Builder::AddAttachment(ImageFormat attachmentFormat)
    {
        const AllocatedImage allocatedImage = ImageBuilder(device)
                .SetResolution(width, height)
                .SetTiling(VK_IMAGE_TILING_OPTIMAL)
                .SetFormat(attachmentFormat)
                .AddUsage(ImageUtils::GetUsageFromFormat(attachmentFormat))
                .Build();

        const VkImageView imageView = ImageViewBuilder(device)
                .SetFormat(attachmentFormat)
                .SetAspectFlags(ImageUtils::GetAspectFlagFromFormat(attachmentFormat))
                .SetImage(allocatedImage.image)
                .Build();

        attachments.push_back({
            allocatedImage,
            imageView,
            VulkanUtils::ConvertImageFormat(attachmentFormat)
        });

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
        std::vector<VkImageView> imageViews;

        for (int i = 0; i < attachments.size(); i++)
        {
            imageViews.push_back(attachments[i].imageView);
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

        return CreateRef<VulkanFramebuffer>(device, framebuffer, width, height, attachments);
    }

    VulkanFramebuffer::~VulkanFramebuffer()
    {
        LOG_INFO("Destroy framebuffer images");
        for (const auto& attachment: attachments)
        {
            if (attachment.allocatedImage.image && attachment.imageView)
            {
                vkDestroyImageView(device->GetDevice(), attachment.imageView, nullptr);
                vmaDestroyImage(device->GetVmaAllocator(), attachment.allocatedImage.image, attachment.allocatedImage.allocation);
            }
        }

        LOG_INFO("Destroy framebuffer");
        vkDestroyFramebuffer(device->GetDevice(), framebuffer, nullptr);
    }
}
