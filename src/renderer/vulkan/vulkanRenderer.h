#pragma once

#include <optional>

#include "renderer/renderer.h"
#include "vulkan/vulkan.h"

class GLFWwindow;

namespace Raytracing
{
	struct SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	struct QueueFamilyIndices
	{
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		[[nodiscard]] bool IsComplete() const { return graphicsFamily.has_value(); }
	};

	class VulkanRenderer : public Renderer
	{
	public:
		VulkanRenderer() = default;
		virtual ~VulkanRenderer() override;

		virtual void Init(int width, int height) override;

		virtual void ProcessPixel(unsigned int pixelCount, vec3 pixelColor) override;
		virtual void OnRenderBegin(const Camera& camera) override;
		virtual void OnRenderFinished(const Camera& camera) override;

		void SetGLFWwindow(GLFWwindow* window)
		{
			glfwWindow = window;
		}

		void CreateSurface();
		void DrawFrame();

	public:
		std::string GetVkResultString(const VkResult vulkan_result);
		std::vector<VkExtensionProperties> GetAvailableExtensions();
		bool CheckIfExtensionSupported(const char* ext);

		std::vector<VkExtensionProperties> GetSupportedDeviceExtensions();
		std::vector<VkLayerProperties> GetSupportedValidationLayers();
		bool CheckIfValidationLayerSupported(const char* layer);


		inline bool CheckDeviceExtensionSupport(std::vector<std::string> deviceExtensions);
		inline bool IsDeviceSuitable(VkPhysicalDevice physicalDevice);

		inline VkDebugUtilsMessengerCreateInfoEXT CreateDebugMessengerCreateInfo();
		inline QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice physicalDevice);
		inline VkPhysicalDevice PickPhysicalDevice();
		inline VkDevice CreateLogicalDevice();
		inline VkQueue GetDeviceQueue();
		inline VkShaderModule CreateShaderModule(const std::vector<char>& code);
		inline SwapChainSupportDetails QuerySwapChainSupport();
		inline VkQueue GetDevicePresentQueue();

	private:
		void InitVulkan();

		void CreateVkInstance(
			const std::vector<const char*>& deviceExtensions,
			const std::vector<const char*>& validationLayers);

		void CreateSwapChain();
		void CreateImageViews();
		void CreateGraphicsPipeline();
		void CreateRenderPass();
		void CreateFramebuffers();
		void CreateCommandPool();
		void CreateCommandBuffer();
		void CreateSyncObjects();


		void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

		static VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		static VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

	private:
		int viewportWidth, viewportHeight;

		GLFWwindow* glfwWindow;

		VkDevice device;
		VkInstance instance;

		VkQueue graphicsQueue;
		VkQueue presentQueue;

		VkSurfaceKHR surface;
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

		VkSwapchainKHR swapChain;
		std::vector<VkImage> swapChainImages;
		VkFormat swapChainImageFormat;
		VkExtent2D swapChainExtent;

		VkRenderPass renderPass;
		VkPipelineLayout pipelineLayout;
		VkPipeline graphicsPipeline;

		VkCommandPool commandPool;
		VkCommandBuffer commandBuffer;

		VkSemaphore imageAvailableSemaphore;
		VkSemaphore renderFinishedSemaphore;
		VkFence inFlightFence;

		std::vector<VkFramebuffer> swapChainFramebuffers;
		std::vector<VkImageView> swapChainImageViews;
	};
}
