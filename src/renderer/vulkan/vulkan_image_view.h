#pragma once
#include "vulkan_image.h"
#include "util/core.h"

namespace Raytracing
{
    class VulkanDevice;

    class VulkanImageView {
    public:
        class Builder {
        public:
            Builder(VulkanDevice* device): device(device) {}
            ~Builder() = default;

            Builder& SetImage(VkImage image);
            Builder& SetFormat(VkFormat format);
            Builder& SetAspectFlags(VkImageAspectFlags flags);
            Builder& SetViewType(VkImageViewType viewType);

            Ref<VulkanImageView> Build();

        private:
            VulkanDevice* device{};
            VkImage image{};
            VkFormat format{};
            VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D;
            VkImageAspectFlags aspectFlags{};
            VkImageView imageView{};
        };

    public:
        VulkanImageView(VulkanDevice* device, VkImageView imageView): device(device), imageView(imageView) {}
        ~VulkanImageView();

        VkImageView Get() { return imageView; }

    private:
        VulkanDevice* device;
        VkImageView imageView;
    };
}
