#pragma once

#include <vulkan/vulkan.h>
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

            VkImage image{};
            VkImageView imageView{};
            VkDeviceMemory imageMemory{};
            VkSampler sampler{};
            ImageResource imageResource{};
        };

    public:
        VulkanTexture(VulkanDevice* _device, const VkImage _image, const VkImageView _imageView, const VkSampler _sampler,
                      const VkDeviceMemory _imageMemory,
                      const ImageResource& _imageResource): device(_device),
                                                            image(_image),
                                                            imageView(_imageView),
                                                            sampler(_sampler),
                                                            imageMemory(_imageMemory),
                                                            imageResource(_imageResource) {}

        ~VulkanTexture();

        VkImage GetImage() const { return image; };
        VkImageView GetImageView() const { return imageView; }
        VkSampler GetSampler() const { return sampler; }
        ImageResource GetImageResource() const { return imageResource; }

    private:
        VulkanDevice* device;
        ImageResource imageResource{};

        VkImage image{};
        VkImageView imageView{};
        VkDeviceMemory imageMemory{};
        VkSampler sampler{};
    };
}
