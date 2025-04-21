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

            VkFormat imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
            VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

            uint32_t imageCount = 0;
        };

    public:
        VulkanSwapchain(VulkanDevice* vulkanDevice, const VkSwapchainKHR swapChain, const VkExtent2D swapChainExtent,
                        const VkFormat swapChainImageFormat,
                        const std::vector<VkImage>& swapChainImages): vulkanDevice(vulkanDevice), swapChain(swapChain),
                                                                                  swapChainExtent(swapChainExtent),
                                                                                  swapChainImageFormat(swapChainImageFormat),
                                                                                  swapChainImages(swapChainImages) {}
        ~VulkanSwapchain();

        VkSwapchainKHR& GetSwapChain() { return swapChain; }
        VkExtent2D& GetExtent() { return swapChainExtent; }
        VkFormat GetImageFormat() const { return swapChainImageFormat; }

        std::vector<VkImage> GetSwapChainImages() const { return swapChainImages; }

    private:
        VulkanDevice* vulkanDevice;
        VkSwapchainKHR swapChain;
        VkExtent2D swapChainExtent;
        VkFormat swapChainImageFormat;
        std::vector<VkImage> swapChainImages;
    };
}
