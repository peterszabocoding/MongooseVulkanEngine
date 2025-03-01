#include "macosVulkanWindow.h"
#include <iostream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "macosVulkanApplication.h"

namespace Raytracing
{
	MacOSVulkanWindow::MacOSVulkanWindow(const AppInfo appInfo, const WindowParams params): Window(params)
	{
		WindowsWindow::OnCreate();
	}

}
