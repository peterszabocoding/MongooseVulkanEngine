#pragma once

#include <string>
#include "util/core.h"
#include "window.h"

namespace Raytracing
{

	struct AppInfo
	{
		std::string appName;
		std::string windowTitle;
		std::string versionName;
		Version appVersion = Version{0, 0, 0};
		int windowWidth = 800;
		int windowHeight = 800;
	};

	class Application
	{
	public:
		static Application* Create();
		static Application& Get() { return *s_Instance; }

		Application();
		virtual ~Application() = default;

		virtual void OnCreate();
		virtual void Run();

		Window& GetWindow() const { return *window; }

	protected:
		AppInfo applicationInfo;

		static Application* s_Instance;
		Scope<Window> window;

		bool isRunning;

		float lastFrameTime = 0.0f;
	};
}
