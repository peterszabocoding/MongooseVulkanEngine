#pragma once

#include "application/application.h"
#include "macosVulkanWindow.h"

namespace Raytracing
{
	class MacOSVulkanApplication : public Application
	{
	public:
		MacOSVulkanApplication();
		~MacOSVulkanApplication() override = default;

		virtual void OnCreate() override;
		virtual void Run() override;

		void CreateWindow();

	private:
		bool isRunning;
		MacOSVulkanWindow* window;
	};
}
