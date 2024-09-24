#include "macosVulkanApplication.h"
#include <iostream>

namespace Raytracing
{
    MacOSVulkanApplication::MacOSVulkanApplication()
    {
		CreateWindow();
    }

    void MacOSVulkanApplication::CreateWindow()
    {
        WindowParams params;
		params.title = applicationInfo.windowTitle.c_str();
		params.width = applicationInfo.windowWidth;
		params.height = applicationInfo.windowHeight;

		window = new MacOSVulkanWindow(applicationInfo, params);
		window->SetOnWindowCloseCallback([&]()
		{
			isRunning = false;
		});
    }

    void MacOSVulkanApplication::OnCreate()
    {
		std::cout << "MacOSVulkanApplication Application OnCreate" << '\n';
		isRunning = true;
	}

	void MacOSVulkanApplication::Run()
	{
		std::cout << "MacOSVulkanApplication Application Run" << '\n';
		while (isRunning) 
			window->OnUpdate();
	}
	
}
