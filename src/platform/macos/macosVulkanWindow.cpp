#include "macosVulkanWindow.h"
#include <iostream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "util/vulkanUtils.h"
#include "macosVulkanApplication.h"

namespace Raytracing
{
	MacOSVulkanWindow::MacOSVulkanWindow(const AppInfo appInfo, const WindowParams params): Window(params)
	{
		applicationInfo = appInfo;
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
		// Check if VK_KHR_portability_subset is supported
		if (!VulkanUtils::CheckIfExtensionSupported(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)) {
			throw std::runtime_error("VK_KHR_portability_subset extension not supported by the device.");
		}

		VulkanUtils::GetSupportedValidationLayers();

		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		VulkanUtils::VulkanInstanceBuilder builder;
		instance = builder
			.ApplicationInfo(applicationInfo)
			.AddDeviceExtension(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)
			.AddDeviceExtensions(glfwExtensions, glfwExtensionCount)
			.AddValidationLayer(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)
			.Build();
	}
}
