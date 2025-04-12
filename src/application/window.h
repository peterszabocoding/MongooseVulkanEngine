#pragma once

#include <functional>

#include "application.h"
#include "GLFW/glfw3.h"
#include "renderer/renderer.h"
#include "renderer/vulkan/imgui_vulkan.h"

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

		virtual void SetOnWindowCloseCallback(OnWindowCloseCallback callback);

	protected:
		AppInfo applicationInfo;
		WindowParams windowParams;

		GLFWwindow* window;
		OnWindowCloseCallback windowCloseCallback;

		Ref<Renderer> renderer;
		Ref<ImGuiVulkan> imGuiVulkan;
	};
}
