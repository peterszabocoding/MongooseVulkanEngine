#pragma once

#include <optional>
#include <vulkan/vulkan.h>
#include "application/window.h"

namespace Raytracing
{
	class WindowsWindow : public Window
	{
	public:
		WindowsWindow(const AppInfo& appInfo, const WindowParams params);
		~WindowsWindow() override;

		virtual void OnCreate() override;
		virtual void OnUpdate() override;

	private:
		void InitVulkan();
		void CreateSurface();
		void CreateSwapChain();
		void CreateImageViews();
		void CreateGraphicsPipeline();

		static VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		static VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

	private:
		AppInfo applicationInfo;
		GLFWwindow* window;

		VkDevice device;
		VkInstance instance;
		VkQueue graphicsQueue;
		VkSurfaceKHR surface;
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

		VkSwapchainKHR swapChain;
		std::vector<VkImage> swapChainImages;
		VkFormat swapChainImageFormat;
		VkExtent2D swapChainExtent;

		std::vector<VkImageView> swapChainImageViews;
	};
}
