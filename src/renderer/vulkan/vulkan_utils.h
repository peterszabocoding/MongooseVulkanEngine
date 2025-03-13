#pragma once

#include <iostream>
#include <stdexcept>
#include <string>
#include <set>
#include <vector>
#include <optional>
#include <vulkan/vulkan_core.h>

namespace Raytracing::VulkanUtils
{
	struct QueueFamilyIndices
	{
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool IsComplete() const { return graphicsFamily.has_value(); }
	};

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		std::cerr << "validation layer: " << pCallbackData->pMessage << '\n';
		return VK_FALSE;
	}

	static std::string GetVkResultString(const VkResult vulkan_result)
	{
		switch (vulkan_result)
		{
		case VK_SUCCESS:
			return "SUCCESS";
		case VK_ERROR_OUT_OF_HOST_MEMORY:
			return "OUT OF HOST MEMORY";
		case VK_ERROR_OUT_OF_DEVICE_MEMORY:
			return "OUT OF DEVICE MEMORY";
		case VK_ERROR_INITIALIZATION_FAILED:
			return "INITIALIZATION FAILED";
		case VK_ERROR_LAYER_NOT_PRESENT:
			return "LAYER NOT PRESENT";
		case VK_ERROR_EXTENSION_NOT_PRESENT:
			return "EXTENSION NOT PRESENT";
		case VK_ERROR_INCOMPATIBLE_DRIVER:
			return "INCOMPATIBLE DRIVER";
		default:
			return "UNKNOWN RESULT '" + std::to_string(vulkan_result);
		}
	}

	static std::vector<VkExtensionProperties> GetAvailableExtensions()
	{
		// List Vulkan available extensions
		uint32_t extension_count = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
		std::vector<VkExtensionProperties> available_extensions(extension_count);
		vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, available_extensions.data());

		std::cout << "available extensions:\n";
		for (const auto& extension : available_extensions)
			std::cout << '\t' << extension.extensionName << '\n';

		return available_extensions;
	}

	static bool CheckIfExtensionSupported(const char* ext)
	{
		const auto available_extensions = VulkanUtils::GetAvailableExtensions();
		for (const auto& extension : available_extensions)
		{
			if (strcmp(extension.extensionName, ext) == 0) return true;
		}
		return false;
	}

	static std::vector<VkExtensionProperties> GetSupportedDeviceExtensions(const VkPhysicalDevice physicalDevice)
	{
		uint32_t extension_count;
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extension_count, nullptr);

		std::vector<VkExtensionProperties> available_extensions(extension_count);
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extension_count, available_extensions.data());

		std::cout << "Supported Device Extensions:" << '\n';
		for (auto [extensionName, specVersion] : available_extensions) std::cout << extensionName << '\n';

		return available_extensions;
	}

	static std::vector<VkLayerProperties> GetSupportedValidationLayers()
	{
		uint32_t layer_count;
		vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

		std::vector<VkLayerProperties> available_layers(layer_count);
		vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

		std::cout << "available validation layers:\n";
		for (const auto& layer : available_layers) std::cout << '\t' << layer.layerName << '\n';

		return available_layers;
	}

	static bool CheckIfValidationLayerSupported(const char* layer)
	{
		const auto validation_layers = GetSupportedValidationLayers();
		for (const auto& lay : validation_layers)
		{
			if (strcmp(lay.layerName, layer) == 0) return true;
		}
		return false;
	}

	static bool CheckDeviceExtensionSupport(const VkPhysicalDevice physicalDevice, std::vector<std::string>& deviceExtensions)
	{
		uint32_t extension_count;
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extension_count, nullptr);

		std::vector<VkExtensionProperties> available_extensions(extension_count);
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extension_count, available_extensions.data());

		std::set required_extensions(deviceExtensions.begin(), deviceExtensions.end());

		for (const auto& [extensionName, specVersion] : available_extensions)
		{
			required_extensions.erase(extensionName);
		}

		return required_extensions.empty();
	}

	static void CheckVkResult(VkResult err)
	{
		if (err == VK_SUCCESS) return;
		throw std::runtime_error("[vulkan] Error: VkResult = " + err);
	}

	static void CheckVkResult(VkResult result, const char* error_msg)
	{
		if (result == VK_SUCCESS) return;
		throw std::runtime_error(error_msg + ' | ' + result);
	}

	static VkDebugUtilsMessengerCreateInfoEXT CreateDebugMessengerCreateInfo()
	{
		VkDebugUtilsMessengerCreateInfoEXT createInfo = {};

		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity =
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

		createInfo.messageType =
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

		createInfo.pfnUserCallback = debugCallback;

		return createInfo;
	}

	static QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
	{
		QueueFamilyIndices indices;

		uint32_t queue_family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queue_family_count, nullptr);

		std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queue_family_count, queue_families.data());

		int i = 0;
		for (const auto& queueFamily : queue_families)
		{
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				indices.graphicsFamily = i;
			}

			VkBool32 presentSupport = 0;
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);

			if (presentSupport)
				indices.presentFamily = i;

			if (indices.IsComplete())
				break;

			i++;
		}

		return indices;
	}

	static bool IsDeviceSuitable(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
	{
		const QueueFamilyIndices indices = FindQueueFamilies(physicalDevice, surface);
		return indices.IsComplete();
	}

	static VkShaderModule CreateShaderModule(const VkDevice device, const std::vector<char>& code)
	{
		VkShaderModuleCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		create_info.codeSize = code.size();
		create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shader_module;
		CheckVkResult(
			vkCreateShaderModule(device, &create_info, nullptr, &shader_module),
			"Failed to create shader module."
		);

		return shader_module;
	}

	static uint32_t FindMemoryType(const VkPhysicalDevice physicalDevice, const uint32_t typeFilter, const VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties mem_properties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &mem_properties);

		for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++)
		{
			if ((typeFilter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}

		throw std::runtime_error("failed to find suitable memory type!");
	}
}
