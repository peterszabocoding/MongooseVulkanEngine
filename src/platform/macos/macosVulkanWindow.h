#pragma once

#include "application/window.h"
#include <vulkan/vulkan.h>

namespace Raytracing
{
	class MacOSVulkanWindow : public Window
	{
	public:
		MacOSVulkanWindow(const WindowParams params);
		virtual ~MacOSVulkanWindow();

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
