#include "windowsWindow.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include "util/filesystem.h"

namespace Raytracing
{
	WindowsWindow::WindowsWindow(const AppInfo& appInfo, const WindowParams params) : Window(params)
	{
		applicationInfo = appInfo;
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window = glfwCreateWindow(windowParams.width, windowParams.height, windowParams.title, nullptr, nullptr);
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		vulkanRenderer.SetGLFWwindow(window);
		vulkanRenderer.Init(width, height);
	}

	WindowsWindow::~WindowsWindow()
	{
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void WindowsWindow::OnCreate()
	{
		std::cout << "Windows window OnCreate" << '\n';
	}

	void WindowsWindow::OnUpdate()
	{
		if (glfwWindowShouldClose(window))
		{
			windowCloseCallback();
			vulkanRenderer.IdleWait();
			return;
		}

		glfwPollEvents();
		vulkanRenderer.DrawFrame();
	}
}
