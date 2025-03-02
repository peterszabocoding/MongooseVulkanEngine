#pragma once
#include "renderer/vulkan/vulkanRenderer.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"

namespace Raytracing
{
	class ImGuiVulkan
	{
	public:
		ImGuiVulkan();
		~ImGuiVulkan();

		void Init(GLFWwindow* glfwWindow, VulkanRenderer* renderer, const int width, const int height);
		void Resize(const int width, const int height);
		void DrawUi() const;

	private:
		void SetupImGui(const int width, const int height);
		void SetupVulkanWindow(int width, int height);
		void ImGuiFrameRender(ImDrawData* draw_data) const;
		void ImGuiFramePresent() const;

	private:
		uint32_t g_MinImageCount = 2;

		VulkanRenderer* renderer = nullptr;
		GLFWwindow* glfwWindow = nullptr;

		ImGui_ImplVulkanH_Window g_MainWindowData;
		ImGui_ImplVulkanH_Window* wd = nullptr;

		VkDebugReportCallbackEXT g_DebugReport = VK_NULL_HANDLE;
		VkPipelineCache g_PipelineCache = VK_NULL_HANDLE;
	};
}
