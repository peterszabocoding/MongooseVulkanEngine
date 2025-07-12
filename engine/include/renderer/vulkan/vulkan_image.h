#pragma once
#include <vulkan/vulkan_core.h>

#include "util/core.h"
#include "vulkan_utils.h"
#include "resource/resource.h"
#include "vma/vk_mem_alloc.h"

namespace MongooseVK
{
    class VulkanDevice;

    struct AllocatedImage {
        uint32_t width, height;
        VkImage image;
        VmaAllocation allocation;
        VmaAllocationInfo allocationInfo;
    };

    namespace ImageUtils {
        inline VkImageAspectFlagBits GetAspectFlagFromFormat(ImageFormat format) {
            if (format == ImageFormat::DEPTH24_STENCIL8 || format == ImageFormat::DEPTH32)
                return VK_IMAGE_ASPECT_DEPTH_BIT;

            return VK_IMAGE_ASPECT_COLOR_BIT;
        }

        inline VkImageUsageFlags GetUsageFromFormat(ImageFormat format) {
            if (format == ImageFormat::DEPTH24_STENCIL8 || format == ImageFormat::DEPTH32)
                return VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

            return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
        }

        inline VkImageLayout GetLayoutFromFormat(ImageFormat format) {
            if (format == ImageFormat::DEPTH24_STENCIL8)
                return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            if (format == ImageFormat::DEPTH32)
                return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;

            return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }
    }

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

        ImageBuilder& SetInitialLayout(VkImageLayout _initialLayout)
        {
            initialLayout = _initialLayout;
            return *this;
        }

        ImageBuilder& SetMipLevels(uint32_t _mipLevels)
        {
            mipLevels = _mipLevels;
            return *this;
        }

        ImageBuilder& SetMSAASamples(VkSampleCountFlagBits _samples)
        {
            samples = _samples;
            return *this;
        }

        ImageBuilder& SetSharingMode(VkSharingMode _sharingMode)
        {
            sharingMode = _sharingMode;
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
        VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        int arrayLayers = 1;
        uint32_t mipLevels = 1;
        VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
        VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE;
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

        ImageViewBuilder& SetMipLevels(uint32_t _mipLevels)
        {
            mipLevels = _mipLevels;
            return *this;
        }

        ImageViewBuilder& SetBaseMipLevel(uint32_t _baseMipLevel)
        {
            baseMipLevel = _baseMipLevel;
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
        uint32_t mipLevels = 1;
        uint32_t baseMipLevel = 0;
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

        ImageSamplerBuilder& SetCompareOp(bool _compareEnabled, VkCompareOp _compareOp)
        {
            compareEnabled = _compareEnabled;
            compareOp = _compareOp;
            return *this;
        }

        ImageSamplerBuilder& SetMipLevels(uint32_t _mipLevels)
        {
            mipLevels = _mipLevels;
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

        bool compareEnabled = false;
        VkCompareOp compareOp = VK_COMPARE_OP_ALWAYS;

        uint32_t mipLevels = 1;
    };
}
