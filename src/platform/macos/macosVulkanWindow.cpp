#include "macosVulkanWindow.h"
#include <iostream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Raytracing
{

	static std::string GetResultString(VkResult vulkan_result)
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
				return "UNKNOWN RESULT '" + vulkan_result;
		}
	}

	MacOSVulkanWindow::MacOSVulkanWindow(const WindowParams params): Window(params)
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window = glfwCreateWindow(windowParams.width, windowParams.height, windowParams.title, nullptr, nullptr);

        InitVulkan();
	}

	MacOSVulkanWindow::~MacOSVulkanWindow()
	{
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void MacOSVulkanWindow::OnCreate()
	{
		std::cout << "Windows window OnCreate" << '\n';
	}

	void MacOSVulkanWindow::OnUpdate()
	{
		if (glfwWindowShouldClose(window)) windowCloseCallback();
		glfwPollEvents();
	}

    void MacOSVulkanWindow::InitVulkan()
	{
		CreateVkInstance();
	}

	void MacOSVulkanWindow::CreateVkInstance()
	{
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = windowParams.title;
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;


		// List Vulkan available extensions
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());
		std::cout << "available extensions:\n";
		for (const auto& extension : availableExtensions)
		{
			std::cout << '\t' << extension.extensionName << '\n';
		}

		// Check if VK_KHR_portability_subset is supported
		bool portabilitySubsetSupported = false;
		for (const auto& extension : availableExtensions) {
			if (strcmp(extension.extensionName, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) == 0) {
				portabilitySubsetSupported = true;
				break;
			}
		}

		if (!portabilitySubsetSupported) {
			throw std::runtime_error("VK_KHR_portability_subset extension not supported by the device.");
		}

		std::vector<const char*> deviceExtensions = {
			VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
		};

		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;

		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		for (size_t i = 0; i < glfwExtensionCount; i++)
		{
			auto ext = glfwExtensions[i];
			deviceExtensions.push_back(ext);
		}

		std::vector<const char*> validationLayers = {};

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		createInfo.enabledExtensionCount =  deviceExtensions.size();
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();

		createInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
		createInfo.enabledLayerCount = validationLayers.size();
		createInfo.ppEnabledLayerNames = validationLayers.data();

		VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
		if (result != VK_SUCCESS)
		{
			std::string message = "ERROR: Instance creation failed with result '";
			message += GetResultString(result);
			message += "'.";
			throw std::runtime_error(message);
		}
	}
}
