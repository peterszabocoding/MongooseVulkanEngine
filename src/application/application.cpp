#include "application.h"

#ifdef PLATFORM_MACOS
#include "platform/macos/macos_application.h"
#endif

#ifdef PLATFORM_WINDOWS
#include "platform/windows/windows_application.h"
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
