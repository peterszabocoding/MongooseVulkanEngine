#pragma once

#include "vulkan_device.h"
#include "vulkan_image.h"

namespace Raytracing
{
    class VulkanSwapchain {
    public:
        void CreateDepthImage(int width, int height);
        VulkanSwapchain(VulkanDevice* vulkanDevice, int width, int height);
        ~VulkanSwapchain();

        void RecreateSwapChain();
        void CleanupSwapChain() const;

        int GetViewportWidth() const { return viewportWidth; }
        int GetViewportHeight() const { return viewportHeight; }

        VkSwapchainKHR& GetSwapChain() { return swapChain; }
        VkExtent2D& GetExtent() { return swapChainExtent; }
        VkFormat GetImageFormat() { return swapChainImageFormat; }

        static VkFormat GetImageFormat(VulkanDevice* device);

        std::vector<VkImage> GetSwapChainImages() const { return swapChainImages; }
        std::vector<VkImageView> GetSwapChainImageViews() const { return swapChainImageViews; }
        std::vector<VkFramebuffer> GetSwapChainFramebuffers() const { return swapChainFramebuffers; }

    private:
        static VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

        static VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

        void CreateSwapChain();

        void CreateImageViews();

        void CreateFramebuffers();

        VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

        static SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice physical_device, VkSurfaceKHR surface);

    private:
        int viewportWidth, viewportHeight;

        VulkanDevice* vulkanDevice;
        Ref<VulkanImage> vulkanDepthImage;

        VkSwapchainKHR swapChain;
        std::vector<VkImage> swapChainImages;
        VkFormat swapChainImageFormat;
        VkExtent2D swapChainExtent;
        std::vector<VkFramebuffer> swapChainFramebuffers;
        std::vector<VkImageView> swapChainImageViews;
    };
}
