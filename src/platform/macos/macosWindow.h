#pragma once

#include "application/window.h"
#include "renderer/vulkan/vulkanRenderer.h"

class AppInfo;

namespace Raytracing
{
	class MacOSWindow : public Window
	{
	public:
		MacOSWindow(const AppInfo appInfo, const WindowParams params);
	};
}
