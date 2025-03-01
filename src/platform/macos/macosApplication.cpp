#include "macosApplication.h"
#include <iostream>

namespace Raytracing
{
    MacOSApplication::MacOSApplication()
    {
		CreateWindow();
    }

    void MacOSApplication::CreateWindow()
    {
        WindowParams params;
		params.title = applicationInfo.windowTitle.c_str();
		params.width = applicationInfo.windowWidth;
		params.height = applicationInfo.windowHeight;

		window = new MacOSWindow(applicationInfo, params);
		window->SetOnWindowCloseCallback([&]()
		{
			isRunning = false;
		});
    }

    void MacOSApplication::OnCreate()
    {
		std::cout << "MacOSVulkanApplication Application OnCreate" << '\n';
		isRunning = true;
	}

	void MacOSApplication::Run()
	{
		std::cout << "MacOSVulkanApplication Application Run" << '\n';
		while (isRunning) 
			window->OnUpdate();
	}
	
}
