#pragma once

#include "application/application.h"
#include <iostream>
#include "windowsWindow.h"

namespace Raytracing
{
	class WindowsApplication : public Application
	{
	public:
		WindowsApplication()
		{
		};
		virtual ~WindowsApplication() = default;

		virtual void OnCreate() override
		{
			std::cout << "Windows Application OnCreate" << std::endl;
			window = new WindowsWindow(WindowParams());

			isRunning = true;
		}

		virtual void Run() override
		{
			std::cout << "Windows Application Run" << std::endl;

			while (isRunning)
			{
				window->OnUpdate();
			}
		}

	private:
		bool isRunning;
		WindowsWindow* window;
	};
}
