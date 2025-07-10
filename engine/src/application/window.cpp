#include "application//window.h"

#include "renderer/vulkan/vulkan_renderer.h"
#include "ui/ui.h"
#include "util/log.h"

namespace MongooseVK
{
    static void FramebufferResizeCallback(GLFWwindow* glfwWindow, int width, int height)
    {
        int currentWidth = width, currentHeight = height;
        glfwGetFramebufferSize(glfwWindow, &currentWidth, &currentHeight);
        while (currentWidth == 0 || currentHeight == 0)
        {
            glfwGetFramebufferSize(glfwWindow, &currentWidth, &currentHeight);
            glfwWaitEvents();
        }

        const auto window = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
        window->Resize(currentWidth, currentHeight);
    }

    Window::Window(const WindowParams params)
    {
        windowParams = params;

        LOG_TRACE("Init GLFW");
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        glfwWindow = glfwCreateWindow(windowParams.width, windowParams.height, windowParams.title, nullptr, nullptr);

        int width, height;
        glfwSetWindowUserPointer(glfwWindow, this);
        glfwGetFramebufferSize(glfwWindow, &width, &height);
        glfwSetFramebufferSizeCallback(glfwWindow, FramebufferResizeCallback);

        LOG_TRACE("Init Vulkan");
        vulkanDevice = VulkanDevice::Create(glfwWindow);
    }

    Window::~Window()
    {
        delete vulkanDevice;

        glfwDestroyWindow(glfwWindow);
        glfwTerminate();
    }

    void Window::OnUpdate(float deltaTime)
    {
        if (glfwWindowShouldClose(glfwWindow))
        {
            windowCloseCallback();
            return;
        }

        glfwPollEvents();
    }

    void Window::Resize(int width, int height)
    {
        if (width == 0 || height == 0)
        {
            glfwGetFramebufferSize(glfwWindow, &width, &height);
            glfwWaitEvents();
        }

        windowParams.width = width;
        windowParams.height = height;
    }

    void Window::SetOnWindowCloseCallback(OnWindowCloseCallback callback)
    {
        windowCloseCallback = std::move(callback);
    }
}
