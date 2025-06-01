#pragma once

#include <vulkan/vulkan.h>

#include "vulkan_image.h"
#include "resource/resource.h"
#include "util/core.h"

namespace Raytracing
{
    class VulkanDevice;

    class VulkanTexture {
    public:
        class Builder {
        public:
            Builder() = default;
            ~Builder() = default;

            Builder& SetData(void* _data, const uint64_t _size)
            {
                data = _data;
                size = _size;
                return *this;
            }

            Builder& SetResolution(const uint32_t _width, const uint32_t _height)
            {
                width = _width;
                height = _height;
                return *this;
            }

            Builder& SetFormat(const ImageFormat _format)
            {
                format = _format;
                return *this;
            }

            Builder& SetFilter(const VkFilter _minFilter, const VkFilter _magFilter)
            {
                minFilter = _minFilter;
                magFilter = _magFilter;
                return *this;
            }

            Builder& SetTiling(const VkImageTiling _tiling)
            {
                tiling = _tiling;
                return *this;
            }

            Builder& AddUsage(const VkImageUsageFlags _usage)
            {
                usage |= _usage;
                return *this;
            }

            Builder& AddAspectFlag(const VkImageAspectFlags _aspectFlags)
            {
                aspectFlags |= _aspectFlags;
                return *this;
            }

            Builder& SetImageResource(ImageResource& _imageResource)
            {
                imageResource = _imageResource;
                return *this;
            }

            Builder& SetMipLevels(uint32_t _mipLevels)
            {
                mipLevels = _mipLevels;
                return *this;
            }

            Builder& SetArrayLayers(uint32_t _arrayLayers)
            {
                arrayLayers = _arrayLayers;
                return *this;
            }

            Ref<VulkanTexture> Build(VulkanDevice* device);

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
            std::vector<VkImageView> imageViews{};
            VkImageView imageView{};
            VkDeviceMemory imageMemory{};
            VkSampler sampler{};
            ImageResource imageResource{};
        };

    public:
        VulkanTexture(VulkanDevice* _device, const AllocatedImage _image, const std::vector<VkImageView> _imageViews, const VkSampler _sampler,
                      const VkDeviceMemory _imageMemory,
                      const ImageResource& _imageResource): device(_device),
                                                            allocatedImage(_image),
                                                            imageViews(_imageViews),
                                                            sampler(_sampler),
                                                            imageMemory(_imageMemory),
                                                            imageResource(_imageResource) {}

        ~VulkanTexture();

        VkImage GetImage() const { return allocatedImage.image; };
        VkImageView GetImageView() const { return imageViews[0]; }
        VkImageView GetImageView(const uint32_t index) { return imageViews[index % imageViews.size()]; }
        VkSampler GetSampler() const { return sampler; }
        ImageResource GetImageResource() const { return imageResource; }

    private:
        VulkanDevice* device;
        ImageResource imageResource{};

        AllocatedImage allocatedImage{};
        std::vector<VkImageView> imageViews{};
        VkDeviceMemory imageMemory{};
        VkSampler sampler{};
    };
}
