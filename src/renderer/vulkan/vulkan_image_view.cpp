#include "vulkan_image_view.h"
#include "vulkan_utils.h"
#include "util/log.h"

namespace Raytracing
{
    VulkanImageView::Builder& VulkanImageView::Builder::SetImage(const VkImage _image)
    {
        image = _image;
        return *this;
    }

    VulkanImageView::Builder& VulkanImageView::Builder::SetFormat(const VkFormat _format)
    {
        format = _format;
        return *this;
    }

    VulkanImageView::Builder& VulkanImageView::Builder::SetAspectFlags(const VkImageAspectFlags _flags)
    {
        aspectFlags = _flags;
        return *this;
    }

    VulkanImageView::Builder& VulkanImageView::Builder::SetViewType(const VkImageViewType _viewType)
    {
        viewType = _viewType;
        return *this;
    }

    Ref<VulkanImageView> VulkanImageView::Builder::Build()
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = viewType;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VK_CHECK_MSG(vkCreateImageView(device->GetDevice(), &viewInfo, nullptr, &imageView), "Failed to create texture image view.");

        return CreateRef<VulkanImageView>(device, imageView);
    }

    VulkanImageView::~VulkanImageView()
    {
        LOG_INFO("Destroy imageview");
        vkDestroyImageView(device->GetDevice(), imageView, nullptr);
    }
}
