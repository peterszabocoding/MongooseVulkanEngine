#pragma once
#include <vulkan/vulkan_core.h>

#include "resource/resource.h"
#include "util/core.h"

namespace Raytracing
{
    class VulkanDevice;
    class VulkanImageView;

    class VulkanImage {
    public:
        class Builder {
        public:
            explicit Builder(VulkanDevice* _device): device(_device) {}
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

            Builder& SetFormat(const VkFormat _format)
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

            Builder& MakeSampler()
            {
                makeSampler = true;
                return *this;
            }

            Ref<VulkanImage> Build();

        private:
            VulkanDevice* device;

            void* data = nullptr;
            uint64_t size = 0;
            uint32_t width = 0;
            uint32_t height = 0;
            VkFilter minFilter = VK_FILTER_LINEAR;
            VkFilter magFilter = VK_FILTER_LINEAR;
            VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
            VkFormat format{};
            VkImageUsageFlags usage = 0;
            VkImageAspectFlags aspectFlags = 0;

            bool makeSampler = false;

            VkImage image{};
            VkDeviceMemory imageMemory{};
            Ref<VulkanImageView> imageView{};
            VkSampler sampler{};
        };

    public:
        VulkanImage(VulkanDevice* device, const VkImage image, Ref<VulkanImageView> imageView): device(device), image(image),
            imageView(imageView) {}

        VulkanImage(VulkanDevice* _device, const VkImage _image, Ref<VulkanImageView> _imageView, VkDeviceMemory _imageMemory,
                    VkSampler _sampler): device(_device), image(_image),
                                         imageView(_imageView), imageMemory(_imageMemory), sampler(_sampler) {}

        virtual ~VulkanImage();

        VkImage GetImage() const { return image; };
        Ref<VulkanImageView> GetImageView() const { return imageView; }
        VkSampler GetSampler() const { return sampler; }

        ImageResource GetImageResource() const { return imageResource; }

        void SetImageResource(const ImageResource& _imageResource)
        {
            imageResource = _imageResource;
        }

    protected:
        VulkanDevice* device;
        VkImage image{};
        Ref<VulkanImageView> imageView{};

        ImageResource imageResource{};
        VkDeviceMemory imageMemory{};
        VkSampler sampler{};
    };
}
