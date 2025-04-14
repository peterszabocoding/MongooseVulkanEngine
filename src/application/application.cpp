#include "application.h"

namespace Raytracing
{
	Application* Application::s_Instance = nullptr;

	Application::Application()
	{
		ASSERT(!s_Instance, "Application has been initialized already");
		s_Instance = this;

		applicationInfo.appName = "RaytracingInOneWeekend";
		applicationInfo.windowWidth = 800;
		applicationInfo.windowHeight = 800;

		Version version;
		version.major = 0;
		version.minor = 1;
		version.patch = 0;

		applicationInfo.appVersion = version;
		applicationInfo.versionName = version.GetVersionName();

		applicationInfo.windowTitle = "Raytracing In One Weekend: " + applicationInfo.versionName;
	}

	void Application::OnCreate()
	{
		std::cout << "Application OnCreate" << '\n';

		WindowParams params;
		params.title = applicationInfo.windowTitle.c_str();
		params.width = applicationInfo.windowWidth;
		params.height = applicationInfo.windowHeight;

		window = new Window(params);
		window->OnCreate();
		window->SetOnWindowCloseCallback([&]()
		{
			isRunning = false;
		});

		isRunning = true;

		std::cout << "Application window created" << '\n';
	}

	void Application::Run()
	{
		std::cout << "Windows Application Run" << '\n';
		while (isRunning)
		{
			float time = glfwGetTime();
			float deltaTime = time - lastFrameTime;
			lastFrameTime = time;

			window->OnUpdate(deltaTime);
		}
	}

	Application* Application::Create()
	{
		return new Application();
	}
}
