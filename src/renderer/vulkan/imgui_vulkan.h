#pragma once
#include "renderer/vulkan/vulkanRenderer.h"

namespace Raytracing
{
	class ImGuiVulkan
	{
	public:
		ImGuiVulkan();
		~ImGuiVulkan();

		void Init(GLFWwindow* glfwWindow, VulkanRenderer* renderer, const int width, const int height);
		void Resize(const int width, const int height);
		void DrawUi();

	private:
		void SetupImGui(const int width, const int height) const;

	private:
		uint32_t g_MinImageCount = 2;

		VulkanRenderer* renderer = nullptr;
		GLFWwindow* glfwWindow = nullptr;

		bool g_SwapChainRebuild = false;
	};
}
