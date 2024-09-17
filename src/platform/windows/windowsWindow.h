#pragma once

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
		GLFWwindow* window;
	};
}
