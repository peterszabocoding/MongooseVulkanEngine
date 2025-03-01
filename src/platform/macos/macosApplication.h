#pragma once

#include "application/application.h"
#include "macosWindow.h"

namespace Raytracing
{
	class MacOSApplication : public Application
	{
	public:
		MacOSApplication();
		~MacOSApplication() override = default;

		virtual void OnCreate() override;
		virtual void Run() override;

		void CreateWindow();

	private:
		bool isRunning;
		MacOSWindow* window;
	};
}
