#include "application.h"
#include "util/log.h"

namespace Raytracing
{
	Application* Application::s_Instance = nullptr;

	Application::Application()
	{
		ASSERT(!s_Instance, "Application has been initialized already");
		s_Instance = this;

		applicationInfo.appName = "RaytracingInOneWeekend";
		applicationInfo.windowWidth = 1675;
		applicationInfo.windowHeight = 935;

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
		LOG_TRACE("Application OnCreate");

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

		LOG_TRACE("Application window created");
	}

	void Application::Run()
	{
		LOG_TRACE("Windows Application Run");
		while (isRunning)
		{
			const float time = glfwGetTime();
			const float deltaTime = time - lastFrameTime;

			lastFrameTime = time;
			window->OnUpdate(deltaTime);
		}
	}

	Application* Application::Create()
	{
		return new Application();
	}
}
