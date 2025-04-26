#include "vulkan_framebuffer.h"

#include <array>

#include "vulkan_device.h"
#include "vulkan_utils.h"
#include "util/log.h"

namespace Raytracing
{
    namespace Utils
    {
        VkFormat ConvertFramebufferAttachmentFormat(FramebufferAttachmentFormat format)
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
            switch (format)
            {
                case FramebufferAttachmentFormat::RGBA8:
                    return VK_IMAGE_ASPECT_COLOR_BIT;
                case FramebufferAttachmentFormat::RGB8:
                    return VK_IMAGE_ASPECT_COLOR_BIT;
                case FramebufferAttachmentFormat::RGBA16F:
                    return VK_IMAGE_ASPECT_COLOR_BIT;
                case FramebufferAttachmentFormat::RGBA32F:
                    return VK_IMAGE_ASPECT_COLOR_BIT;

                case FramebufferAttachmentFormat::DEPTH24_STENCIL8:
                    return VK_IMAGE_ASPECT_DEPTH_BIT;
                case FramebufferAttachmentFormat::DEPTH32:
                    return VK_IMAGE_ASPECT_DEPTH_BIT;

                default:
                    return VK_IMAGE_ASPECT_COLOR_BIT;
            }
        }

        VkImageUsageFlags GetusageFromFormat(FramebufferAttachmentFormat format)
        {
            switch (format)
            {
                case FramebufferAttachmentFormat::RGBA8:
                    return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
                case FramebufferAttachmentFormat::RGB8:
                    return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
                case FramebufferAttachmentFormat::RGBA16F:
                    return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
                case FramebufferAttachmentFormat::RGBA32F:
                    return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

                case FramebufferAttachmentFormat::DEPTH24_STENCIL8:
                    return VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
                case FramebufferAttachmentFormat::DEPTH32:
                    return VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

                default:
                    return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            }
        }
    }

    VulkanFramebuffer::Builder& VulkanFramebuffer::Builder::AddAttachment(VkImageView imageAttachment)
    {
        imageViews.push_back(CreateRef<VulkanImageView>(device, imageAttachment));
        return *this;
    }

    VulkanFramebuffer::Builder& VulkanFramebuffer::Builder::AddAttachment(FramebufferAttachmentFormat attachmentFormat)
    {
        auto image = VulkanImage::Builder(device)
                            .SetFormat(Utils::ConvertFramebufferAttachmentFormat(attachmentFormat))
                            .SetResolution(width, height)
                            .AddAspectFlag(Utils::GetAspectFlagFromFormat(attachmentFormat))
                            .AddUsage(Utils::GetusageFromFormat(attachmentFormat))
                            .AddUsage(VK_IMAGE_USAGE_SAMPLED_BIT)
                            .AddUsage(VK_IMAGE_USAGE_STORAGE_BIT)
                            .MakeSampler()
                            .Build();

        auto imageView = VulkanImageView::Builder(device)
                .SetFormat(Utils::ConvertFramebufferAttachmentFormat(attachmentFormat))
                .SetAspectFlags(Utils::GetAspectFlagFromFormat(attachmentFormat))
                .SetImage(image->GetImage())
                .Build();

        images.push_back(image);
        imageViews.push_back(imageView);

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
        std::vector<VkImageView> attachments{};
        for (const auto& imageView: imageViews)
        {
            attachments.push_back(imageView->Get());
        }

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

        return CreateRef<VulkanFramebuffer>(device, framebuffer, images, imageViews);
    }

    VulkanFramebuffer::~VulkanFramebuffer()
    {
        LOG_INFO("Destroy framebuffer");
        vkDestroyFramebuffer(device->GetDevice(), framebuffer, nullptr);
    }
}
