#pragma once

#include <functional>

#include "GLFW/glfw3.h"
#include "input/camera_controller.h"
#include "renderer/vulkan/imgui_vulkan.h"

namespace MongooseVK
{
    typedef std::function<void()> OnWindowCloseCallback;

    struct WindowParams {
        const char* title;
        uint32_t width;
        uint32_t height;
    };

    class Window {
    public:
        Window(WindowParams params);
        virtual ~Window();

        virtual void OnUpdate(float deltaTime);
        virtual void Resize(int width, int height);

        void SetOnWindowCloseCallback(OnWindowCloseCallback callback);
        void* GetNativeWindow() const { return glfwWindow; }

        uint32_t GetWidth() const { return windowParams.width; }
        uint32_t GetHeight() const { return windowParams.height; }

    protected:
        WindowParams windowParams;

        GLFWwindow* glfwWindow = nullptr;
        OnWindowCloseCallback windowCloseCallback;

        VulkanDevice* vulkanDevice;
    };
}
