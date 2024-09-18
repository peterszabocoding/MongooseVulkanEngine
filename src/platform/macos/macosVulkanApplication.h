#pragma once

#include "application/application.h"
#include "macosVulkanWindow.h"

namespace Raytracing
{
	class MacOSVulkanApplication : public Application
	{
	public:
		MacOSVulkanApplication() = default;
		~MacOSVulkanApplication() override = default;

		virtual void OnCreate() override;
		virtual void Run() override;

	private:
		bool isRunning;
		MacOSVulkanWindow* window;
	};
}
