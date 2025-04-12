#pragma once
#include "renderer/vulkan/vulkan_renderer.h"

namespace Raytracing
{
	class ImGuiVulkan
	{
	public:
		ImGuiVulkan();
		~ImGuiVulkan();

		void Init(GLFWwindow* glfwWindow, VulkanRenderer* renderer, int width, int height);
		void Resize(int width, int height);
		void DrawUi();

	private:
		void SetupImGui(int width, int height) const;

	private:
		VulkanRenderer* renderer = nullptr;
		GLFWwindow* glfwWindow = nullptr;
	};
}
