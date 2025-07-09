#pragma once

#include <array>

#include "util/core.h"
#include "vulkan_image.h"
#include "resource/resource.h"
#include "memory/resource_pool.h"

namespace MongooseVK
{
    class VulkanDevice;

    class VulkanTexture : public PoolObject {
        friend class VulkanTextureBuilder;

    public:
        VulkanTexture(const AllocatedImage& _image, const std::array<VkImageView, 6>& _imageViews,
                      const VkSampler _sampler,
                      const VkDeviceMemory _imageMemory,
                      const ImageResource& _imageResource): imageResource(_imageResource),
                                                            allocatedImage(_image),
                                                            imageViews(_imageViews),
                                                            imageMemory(_imageMemory),
                                                            sampler(_sampler) {}

        ~VulkanTexture() = default;

        VkImageView GetImageView() const { return imageViews[0]; }
        VkImageView GetImageView(const uint32_t index) const { return imageViews[index % imageViews.size()]; }
        VkSampler GetSampler() const { return sampler; }

    public:
        ImageResource imageResource{};
        AllocatedImage allocatedImage{};
        std::array<VkImageView, 6> imageViews{};
        VkDeviceMemory imageMemory{};
        VkSampler sampler{};
    };

    class VulkanTextureBuilder {
    public:
        VulkanTextureBuilder() = default;
        ~VulkanTextureBuilder() = default;

        VulkanTextureBuilder& SetData(void* _data, const uint64_t _size)
        {
            data = _data;
            size = _size;
            return *this;
        }

        VulkanTextureBuilder& SetResolution(const uint32_t _width, const uint32_t _height)
        {
            width = _width;
            height = _height;
            return *this;
        }

        VulkanTextureBuilder& SetFormat(const ImageFormat _format)
        {
            format = _format;
            return *this;
        }

        VulkanTextureBuilder& SetFilter(const VkFilter _minFilter, const VkFilter _magFilter)
        {
            minFilter = _minFilter;
            magFilter = _magFilter;
            return *this;
        }

        VulkanTextureBuilder& SetTiling(const VkImageTiling _tiling)
        {
            tiling = _tiling;
            return *this;
        }

        VulkanTextureBuilder& AddUsage(const VkImageUsageFlags _usage)
        {
            usage |= _usage;
            return *this;
        }

        VulkanTextureBuilder& AddAspectFlag(const VkImageAspectFlags _aspectFlags)
        {
            aspectFlags |= _aspectFlags;
            return *this;
        }

        VulkanTextureBuilder& SetImageResource(ImageResource& _imageResource)
        {
            imageResource = _imageResource;
            return *this;
        }

        VulkanTextureBuilder& SetMipLevels(uint32_t _mipLevels)
        {
            mipLevels = _mipLevels;
            return *this;
        }

        VulkanTextureBuilder& SetArrayLayers(uint32_t _arrayLayers)
        {
            arrayLayers = _arrayLayers;
            return *this;
        }

        Ref<VulkanTexture> Build(VulkanDevice* device);
        void Build(VulkanDevice* device, VulkanTexture& texture);

    private:
        void* data = nullptr;
        uint64_t size = 0;
        uint32_t width = 0;
        uint32_t height = 0;
        VkFilter minFilter = VK_FILTER_LINEAR;
        VkFilter magFilter = VK_FILTER_LINEAR;
        VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
        ImageFormat format = ImageFormat::Unknown;
        VkImageUsageFlags usage = 0;
        VkImageAspectFlags aspectFlags = 0;
        uint32_t mipLevels = 1;
        uint32_t arrayLayers = 1;

        AllocatedImage allocatedImage{};
        std::array<VkImageView, 6> imageViews{};
        VkImageView imageView{};
        VkDeviceMemory imageMemory{};
        VkSampler sampler{};
        ImageResource imageResource{};
    };
}
