#pragma once
#include <string>
#include <vulkan/vulkan_core.h>

namespace Raytracing {
    class VulkanDevice;

    class VulkanImage {
    public:
        VulkanImage(VulkanDevice *device, const std::string &image_path);

        ~VulkanImage();

    private:
        void CreateImage(const std::string &image_path);
        void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
        void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
        void CreateTextureImageView();
        void CreateTextureSampler();

    private:
        VulkanDevice *device;
        VkImage textureImage{};
        VkDeviceMemory textureImageMemory{};
        VkImageView textureImageView{};
        VkSampler textureSampler;
    };
}
