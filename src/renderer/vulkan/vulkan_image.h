#pragma once
#include <vulkan/vulkan_core.h>

#include "vulkan_utils.h"
#include "resource/resource.h"
#include "util/core.h"
#include "vma/vk_mem_alloc.h"

namespace Raytracing
{
    class VulkanDevice;

    struct AllocatedImage {
        VkImage image;
        VmaAllocation allocation;
        VmaAllocationInfo allocationInfo;
    };

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

        ImageBuilder& SetFormat(const ImageFormat _format)
        {
            format = VulkanUtils::ConvertImageFormat(_format);
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

        ImageBuilder& SetArrayLayers(int _arrayLayers)
        {
            ASSERT(_arrayLayers >= 1, "Array layers must be greater than 0.")
            arrayLayers = _arrayLayers;
            return *this;
        }

        ImageBuilder& SetFlags(VkImageCreateFlags _flags)
        {
            flags = _flags;
            return *this;
        }

        AllocatedImage Build();

    private:
        VulkanDevice* device;

        uint32_t width = 0;
        uint32_t height = 0;
        VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
        VkFormat format{};
        VkImageUsageFlags usage = 0;
        VkImageCreateFlags flags = 0;
        int arrayLayers = 1;
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

        ImageViewBuilder& SetFormat(const ImageFormat _format)
        {
            format = VulkanUtils::ConvertImageFormat(_format);
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

        ImageViewBuilder& SetLayerCount(uint32_t _layerCount)
        {
            layerCount = _layerCount;
            return *this;
        }

        ImageViewBuilder& SetBaseArrayLayer(uint32_t _baseArrayLayer)
        {
            baseArrayLayer = _baseArrayLayer;
            return *this;
        }

        VkImageView Build() const;

    private:
        VulkanDevice* device{};
        VkImage image{};
        VkFormat format{};
        VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D;
        VkImageAspectFlags aspectFlags{};
        uint32_t layerCount = 1;
        uint32_t baseArrayLayer = 0;
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

        ImageSamplerBuilder& SetFormat(const ImageFormat _format)
        {
            format = VulkanUtils::ConvertImageFormat(_format);
            return *this;
        }

        ImageSamplerBuilder& SetAddressMode(const VkSamplerAddressMode _addressMode)
        {
            addressMode = _addressMode;
            return *this;
        }

        ImageSamplerBuilder& SetBorderColor(VkBorderColor _borderColor)
        {
            borderColor = _borderColor;
            return *this;
        }

        VkSampler Build();

    private:
        VulkanDevice* device;
        VkFilter minFilter = VK_FILTER_LINEAR;
        VkFilter magFilter = VK_FILTER_LINEAR;
        VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
        VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkBorderColor borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    };
}
