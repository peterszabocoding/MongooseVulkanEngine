#include "vulkan_swapchain.h"

#include <algorithm>
#include <array>

#include "vulkan_device.h"
#include "vulkan_utils.h"

namespace Raytracing {
    VulkanSwapchain::VulkanSwapchain(VulkanDevice* vulkanDevice, int width, int height): vulkanDevice(vulkanDevice),
                                                                                         viewportWidth(width),
                                                                                         viewportHeight(height) {
        vulkanDepthImage = new VulkanDepthImage(vulkanDevice, width, height);
        CreateSwapChain();
        CreateImageViews();
        CreateFramebuffers();
    }

    VulkanSwapchain::~VulkanSwapchain() {
        CleanupSwapChain();
        delete vulkanDepthImage;
    }

    void VulkanSwapchain::CreateSwapChain() {
        const SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(vulkanDevice->GetPhysicalDevice(), vulkanDevice->GetSurface());
        const VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
        const VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
        const VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities);

        uint32_t image_count = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && image_count > swapChainSupport.capabilities.
            maxImageCount) {
            image_count = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = vulkanDevice->GetSurface();

        createInfo.minImageCount = image_count;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        const VulkanUtils::QueueFamilyIndices indices = VulkanUtils::FindQueueFamilies(
            vulkanDevice->GetPhysicalDevice(), vulkanDevice->GetSurface());
        const uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;

        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(vulkanDevice->GetDevice(), &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain!");
        }

        VkResult err = vkGetSwapchainImagesKHR(vulkanDevice->GetDevice(), swapChain, &image_count, nullptr);
        VulkanUtils::CheckVkResult(err);

        swapChainImages.resize(image_count);

        err = vkGetSwapchainImagesKHR(vulkanDevice->GetDevice(), swapChain, &image_count, swapChainImages.data());
        VulkanUtils::CheckVkResult(err);

        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
    }

    void VulkanSwapchain::RecreateSwapChain() {
        vkDeviceWaitIdle(vulkanDevice->GetDevice());

        CleanupSwapChain();

        CreateSwapChain();
        CreateImageViews();

        vulkanDepthImage->Resize(swapChainExtent.width, swapChainExtent.height);
        viewportWidth = swapChainExtent.width;
        viewportHeight = swapChainExtent.height;

        CreateFramebuffers();
    }

    VkExtent2D VulkanSwapchain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) return capabilities.currentExtent;

        VkExtent2D actual_extent = {
            static_cast<uint32_t>(viewportWidth),
            static_cast<uint32_t>(viewportHeight)
        };

        actual_extent.width = std::clamp(
            actual_extent.width,
            capabilities.minImageExtent.width,
            capabilities.maxImageExtent.width);

        actual_extent.height = std::clamp(
            actual_extent.height,
            capabilities.minImageExtent.height,
            capabilities.maxImageExtent.height);

        return actual_extent;
    }

    VkFormat VulkanSwapchain::GetImageFormat(VulkanDevice* device) {
        const SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device->GetPhysicalDevice(), device->GetSurface());
        const VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);

        return surfaceFormat.format;
    }

    VkSurfaceFormatKHR VulkanSwapchain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for (const auto& available_format: availableFormats) {
            if (available_format.format == VK_FORMAT_R8G8B8A8_UNORM
                && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return available_format;
            }
        }
        return availableFormats[0];
    }

    VkPresentModeKHR VulkanSwapchain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        for (const auto& availablePresentMode: availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR || availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
                return availablePresentMode;
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    SwapChainSupportDetails VulkanSwapchain::QuerySwapChainSupport(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
        SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &details.capabilities);

        uint32_t format_count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, nullptr);

        if (format_count != 0) {
            details.formats.resize(format_count);
            vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, details.formats.data());
        }

        uint32_t present_mode_count;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, nullptr);

        if (present_mode_count != 0) {
            details.presentModes.resize(present_mode_count);
            vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, details.presentModes.data());
        }

        return details;
    }

    void VulkanSwapchain::CleanupSwapChain() const {
        for (const auto framebuffer: swapChainFramebuffers)
            vkDestroyFramebuffer(vulkanDevice->GetDevice(), framebuffer, nullptr);

        for (const auto image_view: swapChainImageViews)
            vkDestroyImageView(vulkanDevice->GetDevice(), image_view, nullptr);

        vkDestroySwapchainKHR(vulkanDevice->GetDevice(), swapChain, nullptr);
    }

    void VulkanSwapchain::CreateImageViews() {
        swapChainImageViews.resize(swapChainImages.size());
        for (size_t i = 0; i < swapChainImages.size(); i++)
            swapChainImageViews[i] = VulkanUtils::CreateImageView(vulkanDevice->GetDevice(), swapChainImages[i], swapChainImageFormat,
                                                                  VK_IMAGE_ASPECT_COLOR_BIT);
    }

    void VulkanSwapchain::CreateFramebuffers() {
        swapChainFramebuffers.resize(swapChainImageViews.size());
        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            std::array<VkImageView, 2> attachments = {
                swapChainImageViews[i],
                vulkanDepthImage->GetImageView()
            };

            VkFramebufferCreateInfo framebuffer_info{};
            framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebuffer_info.renderPass = vulkanDevice->GetRenderPass();
            framebuffer_info.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebuffer_info.pAttachments = attachments.data();
            framebuffer_info.width = swapChainExtent.width;
            framebuffer_info.height = swapChainExtent.height;
            framebuffer_info.layers = 1;

            VulkanUtils::CheckVkResult(
                vkCreateFramebuffer(vulkanDevice->GetDevice(), &framebuffer_info, nullptr, &swapChainFramebuffers[i]),
                "failed to create framebuffer!"
            );
        }
    }
}
