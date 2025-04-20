#pragma once;

#include "imgui.h"
#include "renderer/vulkan/imgui_vulkan.h"

namespace Raytracing
{
    class PerformanceWindow : ImGuiWindow {
    public:
        PerformanceWindow(VulkanDevice* device): device(device) {}
        ~PerformanceWindow() override = default;

        virtual const char* GetTitle() override
        {
            return "Performance";
        }

        virtual void Draw() override
        {
            const auto io = ImGui::GetIO();
            const auto deviceProperties = device->GetDeviceProperties();

            ImGui::Text(deviceProperties.deviceName);
            ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        }

    private:
        VulkanDevice* device;
    };

    class CameraSettingsWindow : ImGuiWindow {
    public:
        explicit CameraSettingsWindow(Camera* camera, CameraController& controller): camera(camera), controller(controller) {}
        ~CameraSettingsWindow() override = default;

        virtual const char* GetTitle() override
        {
            return "Camera Settings";
        }

        virtual void Draw() override
        {
            float cameraFov = camera->GetFOV();
            ;
            Utils::DrawFloatControl("FOV", cameraFov, 0.0f, 180.0f, 0.1f, 45.0f, 150.0f);
            Utils::DrawFloatControl("Move speed", controller.movementSpeed, 0.0f, 10.0f, 0.001f, 1.0f, 150.0f);

            camera->SetFOV(cameraFov);
        }

    private:
        Camera* camera;
        CameraController& controller;
    };
}
