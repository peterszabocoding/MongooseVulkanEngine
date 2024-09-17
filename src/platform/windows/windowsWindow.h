#pragma once

#include "application/window.h"
#include <iostream>

namespace Raytracing
{
	class WindowsWindow : public Window
	{
	public:
		WindowsWindow(const WindowParams params): Window(params)
		{
		}

		virtual ~WindowsWindow() = default;

		virtual void OnCreate() override
		{
			std::cout << "Windows window OnCreate" << std::endl;
		}

		virtual void OnUpdate() override
		{
			std::cout << "Windows window OnUpdate" << std::endl;
		}
	};
}
