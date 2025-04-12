#pragma once
#include <vulkan/vulkan_core.h>

#include "util/core.h"

namespace Raytracing
{
    class VulkanDevice;

    class VulkanImage {
    public:
        VulkanImage(VulkanDevice* device, VkImage image, VkDeviceMemory memory, VkImageView imageView, VkSampler sampler)
            : device(device), image(image), imageMemory(memory), imageView(imageView), sampler(sampler) {}

        virtual ~VulkanImage();

        VkImageView GetImageView() const { return imageView; }
        VkSampler GetSampler() const { return sampler; }

    protected:
        VulkanDevice* device;
        VkImage image{};
        VkDeviceMemory imageMemory{};
        VkImageView imageView{};
        VkSampler sampler{};
    };

    class VulkanImageBuilder {
    public:
        VulkanImageBuilder() = default;
        virtual ~VulkanImageBuilder() = default;

        virtual Ref<VulkanImage> Build(VulkanDevice* device) = 0;

        void SetData(unsigned char* data, uint64_t size)
        {
            this->data = data;
            this->size = size;
        }

        void SetResolution(uint32_t width, uint32_t height)
        {
            this->width = width;
            this->height = height;
        }

        void SetFormat(VkFormat format)
        {
            this->format = format;
        }

        void SetFilter(VkFilter minFilter, VkFilter magFilter)
        {
            this->minFilter = minFilter;
            this->magFilter = magFilter;
        }

        void SetTiling(VkImageTiling tiling)
        {
            this->tiling = tiling;
        }

    protected:
        VkDeviceMemory AllocateImageMemory(const VulkanDevice* device, VkImage image, VkMemoryPropertyFlags properties);

        VkImage CreateImage(const VulkanDevice* device, VkImageUsageFlags usage) const;
        VkImageView CreateImageView(const VulkanDevice* device, VkImageAspectFlags aspectFlags) const;
        VkSampler CreateSampler(const VulkanDevice* device) const;

        void TransitionImageLayout(const VulkanDevice* device, VkImageLayout oldLayout, VkImageLayout newLayout) const;
        void CopyBufferToImage(const VulkanDevice* device, VkBuffer buffer) const;

    protected:
        unsigned char* data;
        uint64_t size;
        uint32_t width;
        uint32_t height;
        VkFilter minFilter;
        VkFilter magFilter;
        VkImageTiling tiling;
        VkFormat format;

        VkImage image{};
        VkDeviceMemory imageMemory{};
        VkImageView imageView{};
        VkSampler sampler{};
    };

    class VulkanTextureImageBuilder : public VulkanImageBuilder {
    public:
        VulkanTextureImageBuilder() = default;
        ~VulkanTextureImageBuilder() override = default;

        virtual Ref<VulkanImage> Build(VulkanDevice* device) override;
    };

    class VulkanDepthImageBuilder : public VulkanImageBuilder {
        public:
        VulkanDepthImageBuilder() = default;
        ~VulkanDepthImageBuilder() override = default;

        virtual Ref<VulkanImage> Build(VulkanDevice* device) override;
    };
}
