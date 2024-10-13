#pragma once

#include <string>
#include "util/Core.h"

class Window;

namespace Raytracing
{
	struct AppInfo
	{
		std::string appName;
		std::string windowTitle;
		std::string versionName;
		Version appVersion;
		int windowWidth = 800;
		int windowHeight = 800;
	};

	class Application
	{
	public:
		Application()
		{
			applicationInfo = AppInfo();
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

		virtual ~Application()
		{
		}

		virtual void OnCreate()
		{
		}

		virtual void Run()
		{
		}

		static Application* Create();

	protected:
		AppInfo applicationInfo;
	};
}
