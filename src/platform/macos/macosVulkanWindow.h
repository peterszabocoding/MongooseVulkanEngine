#pragma once

#include "application/window.h"

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
		GLFWwindow* window;
	};
}
