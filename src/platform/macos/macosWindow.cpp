#include "macosWindow.h"
#include <iostream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "macosApplication.h"

namespace Raytracing
{
	MacOSWindow::MacOSWindow(const AppInfo appInfo, const WindowParams params): Window(appInfo, params)
	{
		MacOSWindow::OnCreate();
	}

}
