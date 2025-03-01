#pragma once

#include "application/application.h"
#include "application/window.h"

namespace Raytracing
{
	class WindowsWindow : public Window
	{
	public:
		WindowsWindow(const AppInfo& appInfo, const WindowParams params);
	};
}
