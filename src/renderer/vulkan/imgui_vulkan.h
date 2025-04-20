#pragma once
#include "renderer/vulkan/vulkan_renderer.h"

namespace Raytracing
{

    namespace Utils
    {
        void DrawFloatControl(const std::string& label, float& values, float min, float max, float steps, float resetValue, float columnWidth);
    }

    class ImGuiWindow {
    public:
        ImGuiWindow() = default;
        virtual ~ImGuiWindow() = default;

        virtual const char* GetTitle() = 0;
        virtual void Draw() = 0;
    };

    class ImGuiVulkan {
    public:
        ImGuiVulkan();
        ~ImGuiVulkan();

        void Init(GLFWwindow* glfwWindow, Ref<VulkanRenderer> renderer, int width, int height);
        void Resize(int width, int height);
        void DrawUi();
        void AddWindow(Ref<ImGuiWindow> window) { uiWindows.push_back(window); };

    private:
        void SetupImGui(int width, int height) const;

    private:
        Ref<VulkanRenderer> renderer = nullptr;
        GLFWwindow* glfwWindow = nullptr;

        std::vector<Ref<ImGuiWindow>> uiWindows;
    };
}
