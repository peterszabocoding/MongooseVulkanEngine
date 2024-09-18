#include "macosVulkanWindow.h"
#include <iostream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Raytracing
{
	MacOSVulkanWindow::MacOSVulkanWindow(const WindowParams params): Window(params)
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window = glfwCreateWindow(windowParams.width, windowParams.height, windowParams.title, nullptr, nullptr);
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
}
