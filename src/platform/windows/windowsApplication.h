#pragma once

#include "application/application.h"
#include "windowsWindow.h"

namespace Raytracing
{
	class WindowsApplication : public Application
	{
	public:
		WindowsApplication() = default;
		~WindowsApplication() override = default;

		virtual void OnCreate() override;
		virtual void Run() override;

	private:
		bool isRunning;
		WindowsWindow* window;
	};
}
