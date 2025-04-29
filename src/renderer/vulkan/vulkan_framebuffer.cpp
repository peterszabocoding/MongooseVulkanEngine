#include "vulkan_framebuffer.h"

#include <array>

#include "vulkan_device.h"
#include "vulkan_utils.h"
#include "util/log.h"

namespace Raytracing
{
    namespace Utils
    {
        VkFormat ConvertFramebufferAttachmentFormat(const FramebufferAttachmentFormat format)
        {
            switch (format)
            {
                case FramebufferAttachmentFormat::RGBA8:
                    return VK_FORMAT_R8G8B8A8_UNORM;
                case FramebufferAttachmentFormat::RGB8:
                    return VK_FORMAT_R8G8B8_UNORM;
                case FramebufferAttachmentFormat::RGBA16F:
                    return VK_FORMAT_R16G16B16A16_UNORM;
                case FramebufferAttachmentFormat::RGBA32F:
                    return VK_FORMAT_R32G32B32A32_SINT;

                case FramebufferAttachmentFormat::DEPTH32:
                    return VK_FORMAT_D32_SFLOAT;
                case FramebufferAttachmentFormat::DEPTH24_STENCIL8:
                    return VK_FORMAT_D24_UNORM_S8_UINT;

                default:
                    return VK_FORMAT_UNDEFINED;
            }
        }

        VkImageAspectFlagBits GetAspectFlagFromFormat(FramebufferAttachmentFormat format)
        {
            if (format == FramebufferAttachmentFormat::DEPTH24_STENCIL8 || format == FramebufferAttachmentFormat::DEPTH32)
                return VK_IMAGE_ASPECT_DEPTH_BIT;

            return VK_IMAGE_ASPECT_COLOR_BIT;
        }

        VkImageUsageFlags GetUsageFromFormat(FramebufferAttachmentFormat format)
        {
            if (format == FramebufferAttachmentFormat::DEPTH24_STENCIL8 || format == FramebufferAttachmentFormat::DEPTH32)
                return VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

            return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
        }
    }

    VulkanFramebuffer::Builder& VulkanFramebuffer::Builder::AddAttachment(VkImageView imageAttachment)
    {
        attachments.push_back({nullptr, imageAttachment, nullptr});
        return *this;
    }

    VulkanFramebuffer::Builder& VulkanFramebuffer::Builder::AddAttachment(FramebufferAttachmentFormat attachmentFormat)
    {
        const VkFormat format = Utils::ConvertFramebufferAttachmentFormat(attachmentFormat);
        const AllocatedImage allocatedImage = ImageBuilder(device)
                .SetResolution(width, height)
                .SetTiling(VK_IMAGE_TILING_OPTIMAL)
                .SetFormat(format)
                .AddUsage(Utils::GetUsageFromFormat(attachmentFormat))
                .Build();

        const VkImageView imageView = ImageViewBuilder(device)
                .SetFormat(format)
                .SetAspectFlags(Utils::GetAspectFlagFromFormat(attachmentFormat))
                .SetImage(allocatedImage.image)
                .Build();

        attachments.push_back({allocatedImage.image, imageView, allocatedImage.imageMemory, format});

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
        for (uint32_t i = 0; i < attachments.size(); i++)
        {
            if (attachments[i].imageMemory != VK_NULL_HANDLE)
                vkFreeMemory(device->GetDevice(), attachments[i].imageMemory, nullptr);

            if (attachments[i].imageView != VK_NULL_HANDLE && attachments[i].image != VK_NULL_HANDLE)
            {
                vkDestroyImageView(device->GetDevice(), attachments[i].imageView, nullptr);
                vkDestroyImage(device->GetDevice(), attachments[i].image, nullptr);
            }
        }

        LOG_INFO("Destroy framebuffer");
        vkDestroyFramebuffer(device->GetDevice(), framebuffer, nullptr);
    }
}
