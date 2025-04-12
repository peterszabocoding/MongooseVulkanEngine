#pragma once

#include "application/window.h"
#include "renderer/vulkan/vulkan_renderer.h"

class AppInfo;

namespace Raytracing
{
	class MacOSWindow : public Window
	{
	public:
		MacOSWindow(const AppInfo appInfo, const WindowParams params);
	};
}
