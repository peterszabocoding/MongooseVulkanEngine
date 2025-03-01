#include "application.h"

#ifdef PLATFORM_MACOS
#include "platform/macos/macosApplication.h"
#endif

#ifdef PLATFORM_WINDOWS
#include "platform/windows/windowsApplication.h"
#endif

namespace Raytracing
{
	Application* Application::Create()
	{
	#ifdef PLATFORM_MACOS
			return new MacOSApplication();
	#endif

	#ifdef PLATFORM_WINDOWS
			return new WindowsApplication();
	#endif
	}
}
