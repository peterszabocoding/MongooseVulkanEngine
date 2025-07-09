#include "renderer/vulkan/vulkan_framebuffer.h"

#include <array>

#include "renderer/vulkan/vulkan_device.h"
#include "renderer/vulkan/vulkan_renderpass.h"
#include "renderer/vulkan/vulkan_utils.h"
#include "util/log.h"

namespace MongooseVK {
    VulkanFramebuffer::Builder& VulkanFramebuffer::Builder::AddAttachment(VkImageView imageAttachment) {
        FramebufferAttachment attachment;
        attachment.allocatedImage.image = nullptr;
        attachment.allocatedImage.allocation = nullptr;
        attachment.imageView = imageAttachment;
        attachment.format = VK_FORMAT_UNDEFINED;

        attachments.push_back(attachment);
        return *this;
    }

    VulkanFramebuffer::Builder& VulkanFramebuffer::Builder::AddAttachment(ImageFormat attachmentFormat) {
        const VkImageLayout imageLayout = ImageUtils::GetLayoutFromFormat(attachmentFormat);
        const AllocatedImage allocatedImage = ImageBuilder(device)
                .SetResolution(width, height)
                .SetTiling(VK_IMAGE_TILING_OPTIMAL)
                .SetFormat(attachmentFormat)
                .AddUsage(ImageUtils::GetUsageFromFormat(attachmentFormat))
                .AddUsage(VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
                .SetInitialLayout(imageLayout)
                .Build();

        const VkImageView imageView = ImageViewBuilder(device)
                .SetFormat(attachmentFormat)
                .SetAspectFlags(ImageUtils::GetAspectFlagFromFormat(attachmentFormat))
                .SetImage(allocatedImage.image)
                .Build();

        const VkSampler sampler = ImageSamplerBuilder(device)
                .SetFilter(VK_FILTER_LINEAR, VK_FILTER_LINEAR)
                .SetFormat(attachmentFormat)
                .SetAddressMode(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
                .SetBorderColor(VK_BORDER_COLOR_INT_OPAQUE_BLACK)
                .Build();

        attachments.push_back({
            allocatedImage,
            imageView,
            sampler,
            imageLayout,
            VulkanUtils::ConvertImageFormat(attachmentFormat)
        });

        return *this;
    }

    VulkanFramebuffer::Builder& VulkanFramebuffer::Builder::SetRenderpass(Ref<VulkanRenderPass> _renderPass) {
        renderPass = _renderPass;
        return *this;
    }

    VulkanFramebuffer::Builder& VulkanFramebuffer::Builder::SetResolution(uint32_t _width, uint32_t _height) {
        width = _width;
        height = _height;
        return *this;
    }

    Ref<VulkanFramebuffer> VulkanFramebuffer::Builder::Build() {
        std::vector<VkImageView> imageViews;

        for (int i = 0; i < attachments.size(); i++) {
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

        Params params;
        params.width = width;
        params.height = height;
        params.renderPass = renderPass->Get();
        params.framebuffer = framebuffer;
        params.attachments = attachments;

        return CreateRef<VulkanFramebuffer>(device, params);
    }

    VulkanFramebuffer::~VulkanFramebuffer() {
        LOG_INFO("Destroy framebuffer images");
        for (const auto& attachment: params.attachments) {
            if (attachment.allocatedImage.image && attachment.imageView) {
                vkDestroyImageView(device->GetDevice(), attachment.imageView, nullptr);
                vmaDestroyImage(device->GetVmaAllocator(), attachment.allocatedImage.image, attachment.allocatedImage.allocation);
            }
        }

        LOG_INFO("Destroy framebuffer");
        vkDestroyFramebuffer(device->GetDevice(), params.framebuffer, nullptr);
    }
}
