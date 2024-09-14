#include "application.h"

#ifdef PLATFORM_MACOS
#include "platform/macos/macosApplication.h"
#endif

namespace Raytracing {
    Application* Application::Create()
    {
        #ifdef PLATFORM_MACOS
        return new MacOSApplication();
        #endif

        #ifndef PLATFORM_MACOS
        return nullptr;
        #endif
    }
}