#pragma once

#include "vulkan_device.h"

namespace MongooseVK
{
    class ImageViewBuilder;
    struct VulkanFramebuffer;

    class VulkanSwapchain {
    public:
        class Builder {
        public:
            explicit Builder(VulkanDevice* device): device(device) {}
            ~Builder() = default;

            Builder& SetResolution(int width, int height);
            Builder& SetPresentMode(VkPresentModeKHR presentMode);
            Builder& SetImageFormat(VkFormat _imageFormat);
            Builder& SetImageColorSpace(VkColorSpaceKHR colorSpace);
            Builder& SetImageCount(uint32_t imageCount);

            Scope<VulkanSwapchain> Build();

        private:
            VulkanDevice* device;

            int width = 0, height = 0;
            VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;

            VkSwapchainKHR swapChain{};
            std::vector<VkImage> swapChainImages{};
            std::vector<VkImageView> swapChainImageViews{};

            uint32_t imageCount = 0;
            VkFormat imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
            VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        };

    public:
        VulkanSwapchain(VulkanDevice* vulkanDevice, const VkSwapchainKHR swapChain, const VkExtent2D swapChainExtent,
                        const VkFormat swapChainImageFormat, const std::vector<VkImage>& images,
                        const std::vector<VkImageView> imageViews): vulkanDevice(vulkanDevice),
                                                                     swapChain(swapChain),
                                                                     swapChainExtent(swapChainExtent),
                                                                     swapChainImageFormat(swapChainImageFormat),
                                                                     swapChainImages(images),
                                                                     swapChainImageViews(imageViews) {}

        ~VulkanSwapchain();

        VkSwapchainKHR& GetSwapChain() { return swapChain; }
        VkExtent2D& GetExtent() { return swapChainExtent; }
        VkFormat GetImageFormat() const { return swapChainImageFormat; }

        std::vector<VkImage> GetImages() const { return swapChainImages; }
        std::vector<VkImageView> GetImageViews() const { return swapChainImageViews; }

    private:
        VulkanDevice* vulkanDevice;
        VkSwapchainKHR swapChain;
        VkExtent2D swapChainExtent;
        VkFormat swapChainImageFormat;

        std::vector<VkImage> swapChainImages;
        std::vector<VkImageView> swapChainImageViews;
    };
}
