#pragma once

#include <vulkan/vulkan.h>

#include "application/window.h"

namespace Raytracing
{
	class WindowsWindow : public Window
	{
	public:
		WindowsWindow(const WindowParams params);
		virtual ~WindowsWindow();

		virtual void OnCreate() override;
		virtual void OnUpdate() override;

	private:
		void InitVulkan();
		void CreateVkInstance();

	private:
		GLFWwindow* window;
		VkInstance instance;
	};
}
