#pragma once
#include "renderer/vulkan/vulkan_renderer.h"

namespace Raytracing
{
    namespace ImGuiUtils
    {
        void DrawVec3Control(const std::string& label, glm::vec3& values, bool normalizeVector, float resetValue, float columnWidth);
        void DrawFloatControl(const std::string& label, float& values, float min, float max, float steps,float columnWidth);
        void DrawIntControl(const std::string& label, int& values, int min, int max, float columnWidth);
        void DrawRGBColorPicker(const std::string& label, glm::vec3& values, float columnWidth = 100.0f);
    }

    class ImGuiWindow {
    public:
        ImGuiWindow(Ref<VulkanRenderer> _renderer): renderer(_renderer) {}
        virtual ~ImGuiWindow() = default;

        virtual const char* GetTitle() = 0;
        virtual void Draw() = 0;

        virtual void Resize() {}

    protected:
        Ref<VulkanRenderer> renderer = nullptr;
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

    protected:
        Ref<VulkanRenderer> renderer = nullptr;
        GLFWwindow* glfwWindow = nullptr;

        std::vector<Ref<ImGuiWindow>> uiWindows;
    };
}
