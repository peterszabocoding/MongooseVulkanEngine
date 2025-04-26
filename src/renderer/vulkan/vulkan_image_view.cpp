#include "vulkan_image_view.h"
#include "vulkan_utils.h"
#include "util/log.h"

namespace Raytracing
{
    ImageViewBuilder& ImageViewBuilder::SetImage(const VkImage _image)
    {
        image = _image;
        return *this;
    }

    ImageViewBuilder& ImageViewBuilder::SetFormat(const VkFormat _format)
    {
        format = _format;
        return *this;
    }

    ImageViewBuilder& ImageViewBuilder::SetAspectFlags(const VkImageAspectFlags _flags)
    {
        aspectFlags = _flags;
        return *this;
    }

    ImageViewBuilder& ImageViewBuilder::SetViewType(const VkImageViewType _viewType)
    {
        viewType = _viewType;
        return *this;
    }

    VkImageView ImageViewBuilder::Build() const
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

        VkImageView imageView = VK_NULL_HANDLE;
        VK_CHECK_MSG(vkCreateImageView(device->GetDevice(), &viewInfo, nullptr, &imageView), "Failed to create texture image view.");

        return imageView;
    }
}
