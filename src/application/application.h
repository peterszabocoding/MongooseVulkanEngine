#pragma once

#include <string>

class Window;

namespace Raytracing {

    struct AppVersion {
        int major;
        int minor;
        int patch;

        std::string GetVersionName()
        {
            return std::to_string(major) + '.' + std::to_string(minor) + "." + std::to_string(patch);
        }
    };

    struct AppInfo {
        std::string appName;
        std::string windowTitle;
        std::string versionName;
        AppVersion appVersion;
        int windowWidth = 800;
        int windowHeight = 800;
    };

    class Application {
        public:
            Application() 
            {
                applicationInfo = AppInfo();
                applicationInfo.appName = "RaytracingInOneWeekend";
                applicationInfo.windowWidth = 800;
                applicationInfo.windowHeight = 800;

                AppVersion version;
                version.major = 0;
                version.minor = 1;
                version.patch = 0;

                applicationInfo.appVersion = version;
                applicationInfo.versionName = version.GetVersionName();

                applicationInfo.windowTitle = "Raytracing In One Weekend: " + applicationInfo.versionName;
            }

            virtual ~Application() {}

            virtual void OnCreate() {}
            virtual void Run() {}

            static Application* Create();

        protected:
            AppInfo applicationInfo;
    };
}
