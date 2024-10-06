#include "windowsWindow.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <cstdint>
#include <limits>
#include <algorithm>
#include <iostream>

#include "util/filesystem.h"
#include "util/vulkanUtils.h"

namespace Raytracing
{
	WindowsWindow::WindowsWindow(const AppInfo& appInfo, const WindowParams params) : Window(params)
	{
		applicationInfo = appInfo;
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window = glfwCreateWindow(windowParams.width, windowParams.height, windowParams.title, nullptr, nullptr);

		InitVulkan();
	}

	WindowsWindow::~WindowsWindow()
	{
		vkDestroyInstance(instance, nullptr);
		vkDestroyDevice(device, nullptr);
		glfwDestroyWindow(window);
		glfwTerminate();

		for (const auto image_view : swapChainImageViews)
		{
			vkDestroyImageView(device, image_view, nullptr);
		}
	}

	void WindowsWindow::OnCreate()
	{
		std::cout << "Windows window OnCreate" << '\n';
	}

	void WindowsWindow::OnUpdate()
	{
		if (glfwWindowShouldClose(window)) windowCloseCallback();
		glfwPollEvents();
	}

	void WindowsWindow::InitVulkan()
	{
		VulkanUtils::GetSupportedValidationLayers();

		uint32_t glfw_extension_count = 0;
		const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

		VulkanUtils::VulkanInstanceBuilder builder;
		instance = builder
		           .ApplicationInfo(applicationInfo)
		           .AddDeviceExtensions(glfw_extensions, glfw_extension_count)
		           .AddValidationLayer(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)
		           .Build();

		physicalDevice = VulkanUtils::PickPhysicalDevice(instance);
		device = VulkanUtils::CreateLogicalDevice(physicalDevice);
		graphicsQueue = VulkanUtils::GetDeviceQueue(physicalDevice, device);

		CreateSurface();
		CreateSwapChain();
		CreateImageViews();
		CreateGraphicsPipeline();
	}

	void WindowsWindow::CreateSurface()
	{
		if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
			throw std::runtime_error("failed to create window surface!");
	}

	void WindowsWindow::CreateSwapChain()
	{
		VulkanUtils::SwapChainSupportDetails swapChainSupport =
			VulkanUtils::QuerySwapChainSupport(physicalDevice, surface);

		const VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
		const VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
		const VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities);

		uint32_t image_count = swapChainSupport.capabilities.minImageCount + 1;
		if (swapChainSupport.capabilities.maxImageCount > 0 && image_count > swapChainSupport.capabilities.
			maxImageCount)
		{
			image_count = swapChainSupport.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;

		createInfo.minImageCount = image_count;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		VulkanUtils::QueueFamilyIndices indices = VulkanUtils::FindQueueFamilies(physicalDevice, surface);
		uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

		if (indices.graphicsFamily != indices.presentFamily)
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;

		createInfo.oldSwapchain = VK_NULL_HANDLE;

		if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create swap chain!");
		}

		vkGetSwapchainImagesKHR(device, swapChain, &image_count, nullptr);
		swapChainImages.resize(image_count);
		vkGetSwapchainImagesKHR(device, swapChain, &image_count, swapChainImages.data());

		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent = extent;
	}

	void WindowsWindow::CreateImageViews()
	{
		swapChainImageViews.resize(swapChainImages.size());

		for (size_t i = 0; i < swapChainImages.size(); i++)
		{
			VkImageViewCreateInfo create_info{};
			create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			create_info.image = swapChainImages[i];

			create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
			create_info.format = swapChainImageFormat;

			create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			create_info.subresourceRange.baseMipLevel = 0;
			create_info.subresourceRange.levelCount = 1;
			create_info.subresourceRange.baseArrayLayer = 0;
			create_info.subresourceRange.layerCount = 1;

			if (vkCreateImageView(device, &create_info, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create image views!");
			}
		}
	}

	void WindowsWindow::CreateGraphicsPipeline()
	{
		auto vertShaderCode = FileSystem::ReadFile("shader/spv/vert.spv");
		auto fragShaderCode = FileSystem::ReadFile("shader/spv/frag.spv");

		VkShaderModule vertShaderModule = VulkanUtils::CreateShaderModule(device, vertShaderCode);
		VkShaderModule fragShaderModule = VulkanUtils::CreateShaderModule(device, fragShaderCode);

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

		vkDestroyShaderModule(device, fragShaderModule, nullptr);
		vkDestroyShaderModule(device, vertShaderModule, nullptr);
	}

	VkSurfaceFormatKHR WindowsWindow::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		for (const auto& available_format : availableFormats)
		{
			if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB
				&& available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				return available_format;
			}
		}
		return availableFormats[0];
	}

	VkPresentModeKHR WindowsWindow::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
	{
		for (const auto& availablePresentMode : availablePresentModes)
		{
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				return availablePresentMode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D WindowsWindow::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) return capabilities.currentExtent;

		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		VkExtent2D actual_extent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
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
}
