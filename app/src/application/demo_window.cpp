#include "demo_window.h"
#include "ui/ui.h"

namespace VulkanDemo
{
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

    DemoWindow::DemoWindow(const MongooseVK::WindowParams& params): Window(params)
    {
        LOG_TRACE("Init Renderer");
        renderer.Init(GetWidth(), GetHeight());

        camera.GetTransform().m_Position = glm::vec3(0.0f, 5.0f, 15.0f);
        camera.SetResolution(GetWidth(), GetHeight());
        cameraController = new MongooseVK::CameraController(camera);

        LOG_TRACE("Loading scene...");
        renderer.LoadScene(scenes.DAMAGED_HELMET, environments.NEWPORT_LOFT);

        LOG_TRACE("Init ImGui");
        imGuiVulkan.Init(glfwWindow, &renderer);

        imGuiVulkan.AddWindow(reinterpret_cast<MongooseVK::ImGuiWindow*>(new PerformanceWindow(renderer)));
        imGuiVulkan.AddWindow(reinterpret_cast<MongooseVK::ImGuiWindow*>(new CameraSettingsWindow(renderer, &camera, *cameraController)));
        imGuiVulkan.AddWindow(reinterpret_cast<MongooseVK::ImGuiWindow*>(new GridSettingsWindow(renderer)));
        imGuiVulkan.AddWindow(reinterpret_cast<MongooseVK::ImGuiWindow*>(new LightSettingsWindow(renderer)));
        imGuiVulkan.AddWindow(reinterpret_cast<MongooseVK::ImGuiWindow*>(new PostProcessingWindow(renderer)));
        imGuiVulkan.AddWindow(reinterpret_cast<MongooseVK::ImGuiWindow*>(new GBufferViewer(renderer)));
        imGuiVulkan.AddWindow(reinterpret_cast<MongooseVK::ImGuiWindow*>(new ShadowMapViewer(renderer)));
    }

    DemoWindow::~DemoWindow()
    {
        delete cameraController;
    }

    void DemoWindow::OnUpdate(float deltaTime)
    {
        Window::OnUpdate(deltaTime);
        cameraController->Update(deltaTime);
        imGuiVulkan.DrawUi();
        renderer.Draw(deltaTime, camera);
    }

    void DemoWindow::Resize(int width, int height)
    {
        Window::Resize(width, height);
        if (width > 0 && height > 0)
        {
            camera.SetResolution(width, height);
            imGuiVulkan.Resize();
            renderer.Resize(width, height);
        }
    }
}
