#pragma once
#include <vulkan/vulkan_core.h>

#include "resource/resource.h"
#include "util/core.h"

namespace Raytracing
{
    class VulkanDevice;

    class VulkanImage {
    public:
        VulkanImage(VulkanDevice* device, const VkImage image, const VkDeviceMemory memory, const VkImageView imageView, const VkSampler sampler)
            : device(device), image(image), imageMemory(memory), imageView(imageView), sampler(sampler) {}

        virtual ~VulkanImage();

        void SetImageResource(const ImageResource& imageResource) { this->imageResource = imageResource; };
        VkImageView GetImageView() const { return imageView; }
        VkSampler GetSampler() const { return sampler; }

    protected:
        VulkanDevice* device;
        VkImage image{};
        VkDeviceMemory imageMemory{};
        VkImageView imageView{};
        VkSampler sampler{};
        ImageResource imageResource{};
    };

    class VulkanImageBuilder {
    public:
        VulkanImageBuilder() = default;
        virtual ~VulkanImageBuilder() = default;

        virtual Ref<VulkanImage> Build(VulkanDevice* device) = 0;

        VulkanImageBuilder& SetData(void* data, const uint64_t size)
        {
            this->data = data;
            this->size = size;
            return *this;
        }

        VulkanImageBuilder& SetResolution(const uint32_t width, const uint32_t height)
        {
            this->width = width;
            this->height = height;
            return *this;
        }

        VulkanImageBuilder& SetFormat(const VkFormat format)
        {
            this->format = format;
            return *this;
        }

        VulkanImageBuilder& SetFilter(const VkFilter minFilter, const VkFilter magFilter)
        {
            this->minFilter = minFilter;
            this->magFilter = magFilter;
            return *this;
        }

        VulkanImageBuilder& SetTiling(const VkImageTiling tiling)
        {
            this->tiling = tiling;
            return *this;
        }

    protected:
        VkDeviceMemory AllocateImageMemory(const VulkanDevice* device, VkImage image, VkMemoryPropertyFlags properties);

        VkImage CreateImage(const VulkanDevice* device, VkImageUsageFlags usage) const;
        VkImageView CreateImageView(const VulkanDevice* device, VkImageAspectFlags aspectFlags) const;
        VkSampler CreateSampler(const VulkanDevice* device) const;

        void TransitionImageLayout(VkCommandBuffer commandBuffer, VkImageLayout oldLayout, VkImageLayout newLayout) const;
        void CopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer) const;

    protected:
        void* data;
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
