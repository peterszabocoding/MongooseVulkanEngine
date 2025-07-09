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

    struct Scenes {
        std::string SPONZA = "resources/sponza/Sponza.gltf";
        std::string CANNON = "resources/cannon/cannon.gltf";
        std::string CHESS_GAME = "resources/chess/ABeautifulGame.gltf";
        std::string DAMAGED_HELMET = "resources/DamagedHelmet/DamagedHelmet.gltf";
    } scenes;

    struct HdrEnvironments {
        std::string CLOUDY = "resources/environment/etzwihl_4k.hdr";
        std::string NEWPORT_LOFT = "resources/environment/newport_loft.hdr";
        std::string CASTLE = "resources/environment/champagne_castle_1_4k.hdr";
    } environments;

    Window::Window(const WindowParams params)
    {
        windowParams = params;
    }

    Window::~Window()
    {
        delete vulkanDevice;
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

        LOG_TRACE("Init Vulkan");
        vulkanDevice = VulkanDevice::Create(glfwWindow);
        renderer = CreateRef<VulkanRenderer>();
        imGuiVulkan = CreateRef<ImGuiVulkan>();

        LOG_TRACE("Init Renderer");
        renderer->Init(width, height);

        LOG_TRACE("Init ImGui");
        imGuiVulkan->Init(glfwWindow, VulkanDevice::Get());

        imGuiVulkan->AddWindow(
            std::reinterpret_pointer_cast<ImGuiWindow>(
                CreateRef<PerformanceWindow>(CAST_REF(VulkanRenderer, renderer))));

        imGuiVulkan->AddWindow(
            std::reinterpret_pointer_cast<ImGuiWindow>(
                CreateRef<CameraSettingsWindow>(CAST_REF(VulkanRenderer, renderer), camera.get(), cameraController)));

        imGuiVulkan->AddWindow(
            std::reinterpret_pointer_cast<ImGuiWindow>(
                CreateRef<GridSettingsWindow>(CAST_REF(VulkanRenderer, renderer))));

        imGuiVulkan->AddWindow(
            std::reinterpret_pointer_cast<ImGuiWindow>(
                CreateRef<LightSettingsWindow>(CAST_REF(VulkanRenderer, renderer))));

        imGuiVulkan->AddWindow(
            std::reinterpret_pointer_cast<ImGuiWindow>(
                CreateRef<PostProcessingWindow>(CAST_REF(VulkanRenderer, renderer))));

        imGuiVulkan->AddWindow(
            std::reinterpret_pointer_cast<ImGuiWindow>(
                CreateRef<GBufferViewer>(CAST_REF(VulkanRenderer, renderer))));

        imGuiVulkan->AddWindow(
            std::reinterpret_pointer_cast<ImGuiWindow>(
                CreateRef<ShadowMapViewer>(CAST_REF(VulkanRenderer, renderer))));

        LOG_TRACE("Loading scene...");
        renderer->LoadScene(scenes.CHESS_GAME, environments.CASTLE);

        camera = CreateRef<Camera>(glm::vec3(0.0f, 5.0f, 15.0f));
        camera->SetResolution(width, height);
        cameraController.SetCamera(camera);
    }

    void Window::OnUpdate(float deltaTime)
    {
        if (glfwWindowShouldClose(glfwWindow))
        {
            windowCloseCallback();
            return;
        }

        glfwPollEvents();

        cameraController.Update(deltaTime);
        imGuiVulkan->DrawUi();
        renderer->Draw(deltaTime, camera);
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
            imGuiVulkan->Resize();
            camera->SetResolution(width, height);
        }
    }

    void Window::SetOnWindowCloseCallback(OnWindowCloseCallback callback)
    {
        windowCloseCallback = std::move(callback);
    }
}
