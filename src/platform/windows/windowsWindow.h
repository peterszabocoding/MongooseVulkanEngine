#pragma once

#include "application/application.h"
#include "application/window.h"
#include "renderer/vulkan/vulkanRenderer.h"

namespace Raytracing
{
	class WindowsWindow : public Window
	{
	public:
		WindowsWindow(const AppInfo& appInfo, const WindowParams params);
		~WindowsWindow() override;

		virtual void OnCreate() override;
		virtual void OnUpdate() override;

	private:
		VulkanRenderer vulkanRenderer;
		AppInfo applicationInfo;
		GLFWwindow* window;
	};
}
