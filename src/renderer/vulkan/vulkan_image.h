#pragma once
#include <vulkan/vulkan_core.h>

namespace Raytracing
{
    class VulkanDevice;

    class VulkanImage {
    public:
        explicit VulkanImage(VulkanDevice* device);

        VulkanImage(VulkanDevice* device, VkImage image, VkDeviceMemory memory, VkImageView imageView, VkSampler sampler)
            : device(device), image(image), imageMemory(memory), imageView(imageView), sampler(sampler) {}

        virtual ~VulkanImage();

        VkImageView GetImageView() const { return imageView; }
        VkSampler GetSampler() const { return sampler; }

    protected:
        void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                         VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) const;

    protected:
        void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
        void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
        void CreateTextureImageView(VkFormat, VkImageAspectFlags aspectFlags);
        void CreateTextureSampler();

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

        virtual VulkanImage* Build(VulkanDevice* device) = 0;

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
        VkDeviceMemory AllocateImageMemory(VulkanDevice* device, VkImage image, VkMemoryPropertyFlags properties);

        VkImage CreateImage(VulkanDevice* device, VkImageUsageFlags usage);
        VkImageView CreateImageView(VulkanDevice* device, VkImageAspectFlags aspectFlags);
        VkSampler CreateSampler(VulkanDevice* device);

        void TransitionImageLayout(VulkanDevice* device, VkImageLayout oldLayout, VkImageLayout newLayout);
        void CopyBufferToImage(VulkanDevice* device, VkBuffer buffer);

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

        virtual VulkanImage* Build(VulkanDevice* device) override;
    };
}
