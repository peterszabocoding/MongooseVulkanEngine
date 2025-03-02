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
		ImGuiVulkan(GLFWwindow* glfwWindow, VulkanRenderer* renderer);
		~ImGuiVulkan();

		void Init(const int width, const int height);
		void Resize(const int width, const int height);
		void DrawUi();

	private:
		void SetupImGui(const int width, const int height);
		void SetupVulkanWindow(int width, int height);
		void ImGuiFrameRender(ImGui_ImplVulkanH_Window* wd, ImDrawData* draw_data);
		void ImGuiFramePresent(ImGui_ImplVulkanH_Window* wd);

	private:
		uint32_t g_MinImageCount = 2;

		VulkanRenderer* renderer;
		GLFWwindow* glfwWindow;

		ImGui_ImplVulkanH_Window g_MainWindowData;
		ImGui_ImplVulkanH_Window* wd;

		VkDebugReportCallbackEXT g_DebugReport = VK_NULL_HANDLE;
		VkPipelineCache g_PipelineCache = VK_NULL_HANDLE;
	};
}
