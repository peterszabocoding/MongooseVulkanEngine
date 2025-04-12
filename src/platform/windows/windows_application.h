#pragma once

#include "application/application.h"
#include "windows_window.h"

namespace Raytracing
{
	class WindowsApplication : public Application
	{
	public:
		WindowsApplication() = default;
		~WindowsApplication() override;

		virtual void OnCreate() override;
		virtual void Run() override;

	private:
		bool isRunning;
		WindowsWindow* window;
	};
}
