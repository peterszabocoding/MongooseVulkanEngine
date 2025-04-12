#include "macos_window.h"
#include <iostream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "macos_application.h"

namespace Raytracing
{
	MacOSWindow::MacOSWindow(const AppInfo appInfo, const WindowParams params): Window(appInfo, params)
	{
		MacOSWindow::OnCreate();
	}

}
