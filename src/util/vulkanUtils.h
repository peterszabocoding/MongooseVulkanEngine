#pragma once

#include <set>
#include <string>
#include <vector>
#include <iostream>
#include <vulkan/vulkan.h>

#include "application/application.h"

namespace Raytracing
{
	namespace VulkanUtils
	{
#ifdef NDEBUG
		const bool enableValidationLayers = false;
#else
        const bool enableValidationLayers = true;
#endif

		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData)
		{
			std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
			return VK_FALSE;
		}

		struct SwapChainSupportDetails
		{
			VkSurfaceCapabilitiesKHR capabilities;
			std::vector<VkSurfaceFormatKHR> formats;
			std::vector<VkPresentModeKHR> presentModes;
		};

		struct QueueFamilyIndices
		{
			std::optional<uint32_t> graphicsFamily;
			std::optional<uint32_t> presentFamily;

			[[nodiscard]] bool IsComplete() const { return graphicsFamily.has_value(); }
		};

		inline std::string GetVkResultString(const VkResult vulkan_result)
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

		inline std::vector<VkExtensionProperties> GetAvailableExtensions()
		{
			// List Vulkan available extensions
			uint32_t extension_count = 0;
			vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
			std::vector<VkExtensionProperties> available_extensions(extension_count);
			vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, available_extensions.data());

			std::cout << "available extensions:\n";
			for (const auto& extension : available_extensions) std::cout << '\t' << extension.extensionName << '\n';

			return available_extensions;
		}

		inline bool CheckIfExtensionSupported(const char* ext)
		{
			const auto available_extensions = GetAvailableExtensions();
			for (const auto& extension : available_extensions)
			{
				if (strcmp(extension.extensionName, ext) == 0) return true;
			}
			return false;
		}

		inline std::vector<VkExtensionProperties> GetSupportedDeviceExtensions(const VkPhysicalDevice device)
		{
			uint32_t extension_count;
			vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

			std::vector<VkExtensionProperties> available_extensions(extension_count);
			vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

			std::cout << "Supported Device Extensions:" << '\n';
			for (auto [extensionName, specVersion] : available_extensions) std::cout << extensionName << '\n';

			return available_extensions;
		}

		inline std::vector<VkLayerProperties> GetSupportedValidationLayers()
		{
			uint32_t layer_count;
			vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

			std::vector<VkLayerProperties> available_layers(layer_count);
			vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

			std::cout << "available validation layers:\n";
			for (const auto& layer : available_layers) std::cout << '\t' << layer.layerName << '\n';

			return available_layers;
		}

		inline bool CheckIfValidationLayerSupported(const char* layer)
		{
			const auto validation_layers = GetSupportedValidationLayers();
			for (const auto& lay : validation_layers)
			{
				if (strcmp(lay.layerName, layer) == 0) return true;
			}
			return false;
		}

		inline bool CheckDeviceExtensionSupport(const VkPhysicalDevice physicalDevice,
		                                        std::vector<std::string> deviceExtensions)
		{
			uint32_t extension_count;
			vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extension_count, nullptr);

			std::vector<VkExtensionProperties> available_extensions(extension_count);
			vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extension_count,
			                                     available_extensions.data());

			std::set required_extensions(deviceExtensions.begin(), deviceExtensions.end());

			for (const auto& [extensionName, specVersion] : available_extensions)
			{
				required_extensions.erase(extensionName);
			}

			return required_extensions.empty();
		}

		inline VkDebugUtilsMessengerCreateInfoEXT CreateDebugMessengerCreateInfo()
		{
			VkDebugUtilsMessengerCreateInfoEXT createInfo = {};

			createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			createInfo.pfnUserCallback = debugCallback;

			return createInfo;
		}

		inline QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice device, const VkSurfaceKHR surface = nullptr)
		{
			QueueFamilyIndices indices;

			uint32_t queue_family_count = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

			std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

			int i = 0;
			for (const auto& queueFamily : queue_families)
			{
				if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
					indices.graphicsFamily = i;

				VkBool32 presentSupport = false;
				if (surface) vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
				if (presentSupport) indices.presentFamily = i;
				if (indices.IsComplete()) break;

				i++;
			}

			return indices;
		}

		inline bool IsDeviceSuitable(const VkPhysicalDevice device)
		{
			const QueueFamilyIndices indices = FindQueueFamilies(device);
			return indices.IsComplete();
		}

		inline VkPhysicalDevice PickPhysicalDevice(const VkInstance instance)
		{
			VkPhysicalDevice physical_device = VK_NULL_HANDLE;
			uint32_t device_count = 0;
			vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

			if (device_count == 0) throw std::runtime_error("Failed to find GPUs with Vulkan support!");

			std::vector<VkPhysicalDevice> devices(device_count);
			vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

			for (const auto& device : devices)
			{
				if (IsDeviceSuitable(device))
				{
					physical_device = device;
					break;
				}
			}

			if (physical_device == VK_NULL_HANDLE) throw std::runtime_error("Failed to find a suitable GPU!");

			return physical_device;
		}

		inline VkDevice CreateLogicalDevice(const VkPhysicalDevice physicalDevice)
		{
			VkDevice device;

			const QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);

			constexpr float queue_priority = 1.0f;

			VkDeviceQueueCreateInfo queue_create_info{};
			queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queue_create_info.queueFamilyIndex = indices.graphicsFamily.value();
			queue_create_info.queueCount = 1;
			queue_create_info.pQueuePriorities = &queue_priority;

			VkPhysicalDeviceFeatures deviceFeatures{};
			VkDeviceCreateInfo createInfo{};
			createInfo.pQueueCreateInfos = &queue_create_info;
			createInfo.queueCreateInfoCount = 1;
			createInfo.pEnabledFeatures = &deviceFeatures;

			const std::vector device_extensions = {
				VK_KHR_SWAPCHAIN_EXTENSION_NAME
			};
			createInfo.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
			createInfo.ppEnabledExtensionNames = device_extensions.data();

			if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
				throw std::runtime_error("failed to create logical device!");

			return device;
		}

		inline VkQueue GetDeviceQueue(const VkPhysicalDevice physicalDevice, const VkDevice device)
		{
			VkQueue graphics_queue;
			const QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);
			vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphics_queue);

			return graphics_queue;
		}

		inline SwapChainSupportDetails QuerySwapChainSupport(const VkPhysicalDevice device, const VkSurfaceKHR surface)
		{
			SwapChainSupportDetails details;

			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

			uint32_t format_count;
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);

			if (format_count != 0)
			{
				details.formats.resize(format_count);
				vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, details.formats.data());
			}

			uint32_t present_mode_count;
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);

			if (present_mode_count != 0)
			{
				details.presentModes.resize(present_mode_count);
				vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count,
				                                          details.presentModes.data());
			}

			return details;
		}

		inline VkShaderModule CreateShaderModule(VkDevice device, const std::vector<char>& code)
		{
			VkShaderModuleCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.codeSize = code.size();
			createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

			VkShaderModule shaderModule;
			if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create shader module!");
			}

			return shaderModule;
		}

		class VulkanInstanceBuilder
		{
		public:
			VulkanInstanceBuilder() = default;

			VulkanInstanceBuilder& ApplicationInfo(const AppInfo& appInfo)
			{
				vkApplicationInfo = new VkApplicationInfo();
				vkApplicationInfo->sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
				vkApplicationInfo->pApplicationName = appInfo.windowTitle.c_str();
				vkApplicationInfo->pEngineName = "No Engine";

				vkApplicationInfo->applicationVersion = VK_MAKE_VERSION(appInfo.appVersion.major,
				                                                        appInfo.appVersion.minor,
				                                                        appInfo.appVersion.patch);
				vkApplicationInfo->engineVersion = VK_MAKE_VERSION(appInfo.appVersion.major, appInfo.appVersion.minor,
				                                                   appInfo.appVersion.patch);
				vkApplicationInfo->apiVersion = VK_API_VERSION_1_0;

				createInfo.pApplicationInfo = vkApplicationInfo;
				return *this;
			}

			VulkanInstanceBuilder& AddDeviceExtension(const char* deviceExtension)
			{
				deviceExtensions.push_back(deviceExtension);
				return *this;
			}

			VulkanInstanceBuilder& AddDeviceExtensions(const std::vector<const char*>& deviceExtensionList)
			{
				for (auto ext : deviceExtensionList)
					deviceExtensions.push_back(ext);
				return *this;
			}

			VulkanInstanceBuilder& AddDeviceExtensions(const char** ext, const int count)
			{
				for (int i = 0; i < count; i++)
					deviceExtensions.push_back(ext[i]);
				return *this;
			}

			VulkanInstanceBuilder& AddValidationLayer(const char* validationLayer)
			{
				validationLayers.push_back(validationLayer);
				return *this;
			}

			VulkanInstanceBuilder& AddValidationLayers(const std::vector<const char*>& validationLayerList)
			{
				for (auto val : validationLayerList)
					validationLayers.push_back(val);
				return *this;
			}

			VkInstance Build()
			{
				createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
				createInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

				if (!deviceExtensions.empty())
				{
					createInfo.enabledExtensionCount = deviceExtensions.size();
					createInfo.ppEnabledExtensionNames = deviceExtensions.data();
				}
				else
				{
					createInfo.enabledExtensionCount = 0;
				}

				if (enableValidationLayers && !validationLayers.empty())
				{
					auto debugCreateInfo = CreateDebugMessengerCreateInfo();

					createInfo.enabledLayerCount = validationLayers.size();
					createInfo.ppEnabledLayerNames = validationLayers.data();

					createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
				}
				else
				{
					createInfo.enabledLayerCount = 0;
					createInfo.pNext = nullptr;
				}

				VkInstance instance;
				const VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
				if (result != VK_SUCCESS)
				{
					std::string message = "ERROR: Instance creation failed with result '";
					message += GetVkResultString(result);
					message += "'.";
					throw std::runtime_error(message);
				}

				return instance;
			}

		private:
			VkApplicationInfo* vkApplicationInfo;

			VkInstanceCreateInfo createInfo;
			std::vector<const char*> deviceExtensions;
			std::vector<const char*> validationLayers;
		};
	}
}
