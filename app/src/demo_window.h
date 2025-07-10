#pragma once

#include "application/window.h"

#include <input/camera_controller.h>
#include <renderer/vulkan/imgui_vulkan.h>

class DemoWindow final : public MongooseVK::Window {
public:
    explicit DemoWindow(const MongooseVK::WindowParams& params);

    ~DemoWindow() override;
    virtual void OnUpdate(float deltaTime) override;
    virtual void Resize(int width, int height) override;

private:
    MongooseVK::VulkanRenderer renderer;
    MongooseVK::ImGuiVulkan imGuiVulkan;

    MongooseVK::Camera camera;
    MongooseVK::CameraController* cameraController;
};
