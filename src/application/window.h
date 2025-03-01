#pragma once

#include <functional>
#include <utility>

#include "renderer/renderer.h"

class GLFWwindow;

namespace Raytracing
{
	typedef std::function<void()> OnWindowCloseCallback;

	struct WindowParams
	{
		const char* title;
		unsigned int width;
		unsigned int height;
	};

	class Window
	{
	public:
		Window(AppInfo appInfo, const WindowParams params);
		virtual ~Window();

		virtual void OnCreate();
		virtual void OnUpdate();
		virtual void Resize(int width, int height);

		virtual void SetOnWindowCloseCallback(OnWindowCloseCallback callback)
		{
			windowCloseCallback = std::move(callback);
		}

	protected:
		AppInfo applicationInfo;
		WindowParams windowParams;

		GLFWwindow* window;
		OnWindowCloseCallback windowCloseCallback;

		Renderer* renderer;
	};
}
