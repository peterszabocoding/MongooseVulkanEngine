#pragma once

#include "vulkan_device.h"
#include "vulkan_image.h"

namespace Raytracing
{
    class VulkanImageView;
    class VulkanFramebuffer;

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    class VulkanSwapchain {
    public:
        class Builder {
        public:
            Builder(VulkanDevice* device): device(device) {}
            ~Builder() = default;

            Builder& SetResolution(int width, int height);
            Builder& SetPresentMode(VkPresentModeKHR presentMode);
            Builder& SetImageFormat(VkFormat imageFormat);
            Builder& SetImageColorSpace(VkColorSpaceKHR colorSpace);
            Builder& SetImageCount(uint32_t imageCount);

            Scope<VulkanSwapchain> Build();

        private:
            VulkanDevice* device;

            int width = 0, height = 0;
            VkPresentModeKHR presentMode;

            VkSwapchainKHR swapChain;
            std::vector<VkImage> swapChainImages;
            std::vector<Ref<VulkanImageView>> swapChainImageViews;

            VkFormat imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
            VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

            uint32_t imageCount = 0;
        };

    public:
        VulkanSwapchain(VulkanDevice* vulkanDevice, const VkSwapchainKHR swapChain, const VkExtent2D swapChainExtent,
                        const VkFormat swapChainImageFormat,
                        const std::vector<VkImage>& images, const std::vector<Ref<VulkanImageView>>& imageViews): vulkanDevice(vulkanDevice), swapChain(swapChain),
                                                                     swapChainExtent(swapChainExtent),
                                                                     swapChainImageFormat(swapChainImageFormat),
                                                                     images(images), imageViews(imageViews) {}

        ~VulkanSwapchain();

        VkSwapchainKHR& GetSwapChain() { return swapChain; }
        VkExtent2D& GetExtent() { return swapChainExtent; }
        VkFormat GetImageFormat() const { return swapChainImageFormat; }

        std::vector<VkImage> GetImages() const { return images; }
        std::vector<Ref<VulkanImageView>> GetImageViews() const { return imageViews; }

    private:
        VulkanDevice* vulkanDevice;
        VkSwapchainKHR swapChain;
        VkExtent2D swapChainExtent;
        VkFormat swapChainImageFormat;

        std::vector<VkImage> images;
        std::vector<Ref<VulkanImageView>> imageViews;
    };
}
