#include "windowsWindow.h"

namespace Raytracing
{
	WindowsWindow::WindowsWindow(const AppInfo& appInfo, const WindowParams params) : Window(appInfo, params)
	{
		WindowsWindow::OnCreate();
	}
}
