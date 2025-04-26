#pragma once
#include "vulkan_image.h"
#include "util/core.h"

namespace Raytracing
{
    class VulkanDevice;

    class ImageViewBuilder {
    public:
        ImageViewBuilder(VulkanDevice* device): device(device) {}
        ~ImageViewBuilder() = default;

        ImageViewBuilder& SetImage(VkImage image);
        ImageViewBuilder& SetFormat(VkFormat format);
        ImageViewBuilder& SetAspectFlags(VkImageAspectFlags flags);
        ImageViewBuilder& SetViewType(VkImageViewType viewType);

        VkImageView Build() const;

    private:
        VulkanDevice* device{};
        VkImage image{};
        VkFormat format{};
        VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D;
        VkImageAspectFlags aspectFlags{};
    };
}
