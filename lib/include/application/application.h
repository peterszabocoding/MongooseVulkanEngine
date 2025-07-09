#pragma once

#include <string>
#include "util/core.h"
#include "window.h"

namespace MongooseVK
{

	struct ApplicationConfig
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
		static Application* Create(const ApplicationConfig& config);
		static Application& Get() { return *s_Instance; }

		Application(ApplicationConfig config);
		virtual ~Application() = default;

		virtual void OnCreate();
		virtual void Run();

		Window& GetWindow() const { return *window; }

	protected:
		ApplicationConfig applicationInfo;

		static Application* s_Instance;
		Scope<Window> window;

		bool isRunning;
		float lastFrameTime = 0.0f;
	};
}
