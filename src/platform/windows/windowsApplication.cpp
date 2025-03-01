#include "windowsApplication.h"
#include <iostream>

namespace Raytracing
{
	WindowsApplication::~WindowsApplication()
	{
		delete window;
	}

	void WindowsApplication::OnCreate()
	{
		std::cout << "Windows Application OnCreate" << '\n';

		WindowParams params;
		params.title = applicationInfo.windowTitle.c_str();
		params.width = applicationInfo.windowWidth;
		params.height = applicationInfo.windowHeight;

		window = new WindowsWindow(applicationInfo, params);
		window->SetOnWindowCloseCallback([&]()
		{
			isRunning = false;
		});

		isRunning = true;
	}

	void WindowsApplication::Run()
	{
		std::cout << "Windows Application Run" << '\n';

		while (isRunning)
		{
			window->OnUpdate();
		}
	}
}
