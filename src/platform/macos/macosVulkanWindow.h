#pragma once

#include "application/window.h"
#include "renderer/vulkan/vulkanRenderer.h"

class AppInfo;

namespace Raytracing
{
	class MacOSVulkanWindow : public Window
	{
	public:
		MacOSVulkanWindow(const AppInfo appInfo, const WindowParams params);
	};
}
