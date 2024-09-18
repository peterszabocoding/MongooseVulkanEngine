#include "macosVulkanApplication.h"
#include <iostream>

namespace Raytracing
{
	void MacOSVulkanApplication::OnCreate()
	{
		std::cout << "MacOSVulkanApplication Application OnCreate" << '\n';
		window = new MacOSVulkanWindow(WindowParams());
		window->SetOnWindowCloseCallback([&]()
		{
			isRunning = false;
		});

		isRunning = true;
	}

	void MacOSVulkanApplication::Run()
	{
		std::cout << "MacOSVulkanApplication Application Run" << '\n';

		while (isRunning)
		{
			window->OnUpdate();
		}
	}
}
