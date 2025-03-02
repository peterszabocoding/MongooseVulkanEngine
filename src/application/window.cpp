#include "window.h"

#include "GLFW/glfw3.h"
#include "renderer/vulkan/vulkanRenderer.h"

namespace Raytracing
{
	static void FramebufferResizeCallback(GLFWwindow* glfwWindow, int width, int height)
	{
		int currentWidth = width, currentHeight = height;
		glfwGetFramebufferSize(glfwWindow, &currentWidth, &currentHeight);
		while (currentWidth == 0 || currentHeight == 0)
		{
			glfwGetFramebufferSize(glfwWindow, &currentWidth, &currentHeight);
			glfwWaitEvents();
		}

		const auto window = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
		window->Resize(currentWidth, currentHeight);
	}

	Window::Window(AppInfo appInfo, const WindowParams params)
	{
		applicationInfo = std::move(appInfo);
		windowParams = params;
		renderer = new VulkanRenderer();
	}

	Window::~Window()
	{
		glfwDestroyWindow(window);
		glfwTerminate();
		delete renderer;
	}

	void Window::OnCreate()
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		window = glfwCreateWindow(windowParams.width, windowParams.height, windowParams.title, nullptr, nullptr);

		int width, height;
		glfwSetWindowUserPointer(window, this);
		glfwGetFramebufferSize(window, &width, &height);
		glfwSetFramebufferSizeCallback(window, FramebufferResizeCallback);

		renderer->SetGLFWwindow(window);
		renderer->Init(width, height);

		imGuiVulkan = new ImGuiVulkan(window, dynamic_cast<VulkanRenderer*>(renderer));
		imGuiVulkan->Init(width, height);
	}

	void Window::OnUpdate()
	{
		if (glfwWindowShouldClose(window))
		{
			windowCloseCallback();
			renderer->IdleWait();
			return;
		}

		glfwPollEvents();
		renderer->DrawFrame();
		imGuiVulkan->DrawUi();
	}

	void Window::Resize(int width, int height)
	{
		if (width == 0 || height == 0)
		{
			glfwGetFramebufferSize(window, &width, &height);
			glfwWaitEvents();
		}
		else
		{
			renderer->Resize(width, height);
			imGuiVulkan->Resize(width, height);
		}
	}
}
