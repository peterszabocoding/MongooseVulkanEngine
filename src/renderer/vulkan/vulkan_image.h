#pragma once
#include <vulkan/vulkan_core.h>

namespace Raytracing
{
    class VulkanDevice;
    class ImageViewBuilder;

    class ImageBuilder {
    public:
        ImageBuilder(VulkanDevice* _device): device(_device) {}
        ~ImageBuilder() = default;

        ImageBuilder& SetResolution(const uint32_t _width, const uint32_t _height)
        {
            width = _width;
            height = _height;
            return *this;
        }

        ImageBuilder& SetFormat(const VkFormat _format)
        {
            format = _format;
            return *this;
        }

        ImageBuilder& SetTiling(const VkImageTiling _tiling)
        {
            tiling = _tiling;
            return *this;
        }

        ImageBuilder& AddUsage(const VkImageUsageFlags _usage)
        {
            usage |= _usage;
            return *this;
        }

        VkImage Build(VkDeviceMemory& imageMemory);

    private:
        VulkanDevice* device;

        uint32_t width = 0;
        uint32_t height = 0;
        VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
        VkFormat format{};
        VkImageUsageFlags usage = 0;
    };

    class ImageViewBuilder {
    public:
        ImageViewBuilder(VulkanDevice* device): device(device) {}
        ~ImageViewBuilder() = default;

        ImageViewBuilder& SetImage(const VkImage _image)
        {
            image = _image;
            return *this;
        }

        ImageViewBuilder& SetFormat(const VkFormat _format)
        {
            format = _format;
            return *this;
        }

        ImageViewBuilder& SetAspectFlags(const VkImageAspectFlags _flags)
        {
            aspectFlags = _flags;
            return *this;
        }

        ImageViewBuilder& SetViewType(const VkImageViewType _viewType)
        {
            viewType = _viewType;
            return *this;
        }

        VkImageView Build() const;

    private:
        VulkanDevice* device{};
        VkImage image{};
        VkFormat format{};
        VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D;
        VkImageAspectFlags aspectFlags{};
    };

    class ImageSamplerBuilder {
    public:
        ImageSamplerBuilder(VulkanDevice* _device): device(_device) {}
        ~ImageSamplerBuilder() = default;

        ImageSamplerBuilder& SetFilter(const VkFilter _minFilter, const VkFilter _magFilter)
        {
            minFilter = _minFilter;
            magFilter = _magFilter;
            return *this;
        }

        VkSampler Build();

    private:
        VulkanDevice* device;
        VkFilter minFilter = VK_FILTER_LINEAR;
        VkFilter magFilter = VK_FILTER_LINEAR;
    };
}
