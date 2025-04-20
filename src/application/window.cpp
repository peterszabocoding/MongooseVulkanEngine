#include "window.h"

#include "renderer/vulkan/vulkan_renderer.h"
#include "ui/ui.h"
#include "util/log.h"

namespace Raytracing
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
        renderer = Renderer::Create();
        imGuiVulkan = CreateRef<ImGuiVulkan>();
    }

    Window::~Window()
    {
        glfwDestroyWindow(glfwWindow);
        glfwTerminate();
    }

    void Window::OnCreate()
    {
        LOG_TRACE("Init GLFW");
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        glfwWindow = glfwCreateWindow(windowParams.width, windowParams.height, windowParams.title, nullptr, nullptr);

        int width, height;
        glfwSetWindowUserPointer(glfwWindow, this);
        glfwGetFramebufferSize(glfwWindow, &width, &height);
        glfwSetFramebufferSizeCallback(glfwWindow, FramebufferResizeCallback);

        LOG_TRACE("Init Renderer");
        renderer->SetGLFWwindow(glfwWindow);
        renderer->Init(width, height);

        LOG_TRACE("Init ImGui");
        imGuiVulkan->Init(glfwWindow, CAST_REF(VulkanRenderer, renderer), width, height);

        camera = CreateRef<Camera>();
        camera->SetResolution(width, height);
        camera->GetTransform().m_Position = glm::vec3(0.0f, 0.0f, 2.0f);

        cameraController.SetCamera(camera);

        imGuiVulkan->AddWindow(
            std::reinterpret_pointer_cast<ImGuiWindow>(
                CreateRef<PerformanceWindow>(CAST_REF(VulkanRenderer, renderer)->GetVulkanDevice())));
        imGuiVulkan->AddWindow(std::reinterpret_pointer_cast<ImGuiWindow>(CreateRef<CameraSettingsWindow>(camera.get(), cameraController)));
    }

    void Window::OnUpdate(float deltaTime)
    {
        if (glfwWindowShouldClose(glfwWindow))
        {
            windowCloseCallback();
            renderer->IdleWait();
            return;
        }

        glfwPollEvents();

        cameraController.Update(deltaTime);

        imGuiVulkan->DrawUi();
        renderer->DrawFrame(deltaTime, camera);
    }

    void Window::Resize(int width, int height)
    {
        if (width == 0 || height == 0)
        {
            glfwGetFramebufferSize(glfwWindow, &width, &height);
            glfwWaitEvents();
        } else
        {
            renderer->Resize(width, height);
            imGuiVulkan->Resize(width, height);
            camera->SetResolution(width, height);
        }
    }

    void Window::SetOnWindowCloseCallback(OnWindowCloseCallback callback)
    {
        windowCloseCallback = std::move(callback);
    }
}
