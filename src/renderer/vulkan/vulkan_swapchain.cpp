#include "vulkan_swapchain.h"

#include <algorithm>
#include <array>

#include "vulkan_device.h"
#include "vulkan_framebuffer.h"
#include "vulkan_utils.h"
#include "util/log.h"

namespace Raytracing
{
    namespace ImageUtils
    {
        VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, int width, int height)
        {
            if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
                return capabilities.currentExtent;

            VkExtent2D extent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            extent.width = std::clamp(
                extent.width,
                capabilities.minImageExtent.width,
                capabilities.maxImageExtent.width);

            extent.height = std::clamp(
                extent.height,
                capabilities.minImageExtent.height,
                capabilities.maxImageExtent.height);

            return extent;
        }
    }

    VulkanSwapchain::Builder& VulkanSwapchain::Builder::SetResolution(int _width, int _height)
    {
        width = _width;
        height = _height;
        return *this;
    }

    VulkanSwapchain::Builder& VulkanSwapchain::Builder::SetPresentMode(VkPresentModeKHR _presentMode)
    {
        presentMode = _presentMode;
        return *this;
    }

    VulkanSwapchain::Builder& VulkanSwapchain::Builder::SetImageFormat(VkFormat _imageFormat)
    {
        imageFormat = _imageFormat;
        return *this;
    }

    VulkanSwapchain::Builder& VulkanSwapchain::Builder::SetImageColorSpace(VkColorSpaceKHR _colorSpace)
    {
        colorSpace = _colorSpace;
        return *this;
    }

    VulkanSwapchain::Builder& VulkanSwapchain::Builder::SetImageCount(uint32_t _imageCount)
    {
        imageCount = _imageCount;
        return *this;
    }

    Scope<VulkanSwapchain> VulkanSwapchain::Builder::Build()
    {
        const VulkanUtils::SwapChainSupportDetails swapChainSupport = VulkanUtils::QuerySwapChainSupport(
            device->GetPhysicalDevice(),
            device->GetSurface());

        const VkExtent2D extent = ImageUtils::ChooseSwapExtent(swapChainSupport.capabilities, width, height);

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = device->GetSurface();
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = imageFormat;
        createInfo.imageColorSpace = colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        swapChainImages.resize(imageCount);

        VK_CHECK_MSG(vkCreateSwapchainKHR(device->GetDevice(), &createInfo, nullptr, &swapChain),
                     "Failed to create swap chain.");
        VK_CHECK_MSG(vkGetSwapchainImagesKHR(device->GetDevice(), swapChain, &imageCount, swapChainImages.data()),
                     "Failed to acquire swapchain images.");

        swapChainImageViews.resize(imageCount);

        for (size_t i = 0; i < imageCount; i++)
        {
            swapChainImageViews[i] = ImageViewBuilder(device)
                    .SetFormat(imageFormat)
                    .SetImage(swapChainImages[i])
                    .SetAspectFlags(VK_IMAGE_ASPECT_COLOR_BIT)
                    .SetViewType(VK_IMAGE_VIEW_TYPE_2D)
                    .Build();
        }

        return CreateScope<VulkanSwapchain>(device, swapChain, extent, imageFormat, swapChainImages, swapChainImageViews);
    }

    VulkanSwapchain::~VulkanSwapchain()
    {
        LOG_INFO("Destroy swapchain");
        for (const auto& imageView : swapChainImageViews)
            vkDestroyImageView(vulkanDevice->GetDevice(), imageView, nullptr);

        vkDestroySwapchainKHR(vulkanDevice->GetDevice(), swapChain, nullptr);
    }
}
