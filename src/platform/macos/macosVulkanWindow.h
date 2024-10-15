#pragma once

#include <vector>
#include "application/window.h"
#include <vulkan/vulkan.h>
#include "renderer/vulkan/vulkanRenderer.h"

class AppInfo;

namespace Raytracing
{
	class MacOSVulkanWindow : public Window
	{
	public:
		MacOSVulkanWindow(const AppInfo appInfo, const WindowParams params);
		virtual ~MacOSVulkanWindow();

		virtual void OnCreate() override;
		virtual void OnUpdate() override;

	private:
		GLFWwindow* window;
        VkInstance instance;
		AppInfo applicationInfo;
		VulkanRenderer vulkanRenderer;
	};
}
