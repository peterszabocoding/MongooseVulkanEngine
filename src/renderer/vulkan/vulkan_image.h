#pragma once
#include <vulkan/vulkan_core.h>

namespace Raytracing
{
    class VulkanDevice;

    class VulkanImage {
    public:
        explicit VulkanImage(VulkanDevice *device);
        virtual ~VulkanImage();

        VkImageView GetImageView() const { return textureImageView; }
        VkSampler GetSampler() const { return textureSampler; }

    protected:
        void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                         VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory) const;

    protected:
        void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
        void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
        void CreateTextureImageView(VkFormat , VkImageAspectFlags aspectFlags);
        void CreateTextureSampler();

    protected:
        VulkanDevice *device;
        VkImage textureImage{};
        VkDeviceMemory textureImageMemory{};
        VkImageView textureImageView{};
        VkSampler textureSampler{};
    };
}
