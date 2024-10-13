#pragma once

#include <functional>

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
		Window(const WindowParams params)
		{
			windowParams = params;
		}

		virtual ~Window() = default;

		virtual void OnCreate()
		{
		}

		virtual void OnUpdate()
		{
		}

		virtual void SetOnWindowCloseCallback(OnWindowCloseCallback callback)
		{
			windowCloseCallback = callback;
		}

		static Window* Create(const WindowParams params = WindowParams());

	protected:
		WindowParams windowParams;
		OnWindowCloseCallback windowCloseCallback;
	};
}
