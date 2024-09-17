#include "windowsApplication.h"
#include <iostream>

namespace Raytracing
{
	void WindowsApplication::OnCreate()
	{
		std::cout << "Windows Application OnCreate" << '\n';
		window = new WindowsWindow(WindowParams());
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
