#pragma once

#include <functional>

#include "GLFW/glfw3.h"
#include "input/camera_controller.h"
#include "renderer/renderer.h"
#include "renderer/vulkan/imgui_vulkan.h"

namespace Raytracing
{
    typedef std::function<void()> OnWindowCloseCallback;

    struct WindowParams {
        const char* title;
        unsigned int width;
        unsigned int height;
    };

    class Window {
    public:
        Window(WindowParams params);
        ~Window();

        void OnCreate();
        void OnUpdate(float deltaTime);
        void Resize(int width, int height);

        void SetOnWindowCloseCallback(OnWindowCloseCallback callback);

        void* GetNativeWindow() const { return glfwWindow; }

    protected:
        WindowParams windowParams;

        GLFWwindow* glfwWindow = nullptr;
        OnWindowCloseCallback windowCloseCallback;

        Ref<Renderer> renderer;
        Ref<ImGuiVulkan> imGuiVulkan;

        CameraController cameraController;
        Ref<Camera> camera;
    };
}
