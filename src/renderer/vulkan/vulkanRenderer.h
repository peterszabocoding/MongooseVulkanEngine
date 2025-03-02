#pragma once

#define GLFW_INCLUDE_VULKAN

#include <optional>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"

#include "renderer/renderer.h"
#include "vulkan/vulkan.h"

class GLFWwindow;

namespace Raytracing
{
	constexpr int MAX_FRAMES_IN_FLIGHT = 2;

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
		virtual void SetupImGui(const int width, const int height) override;

		virtual void ProcessPixel(unsigned int pixelCount, vec3 pixelColor) override;
		virtual void OnRenderBegin(const Camera& camera) override;
		virtual void OnRenderFinished(const Camera& camera) override;

		virtual void IdleWait() override;
		virtual void Resize(int width, int height) override;
		virtual void DrawFrame() override;
		virtual void DrawUi() override;

		void CreateSurface();

	public:
		static std::string GetVkResultString(const VkResult vulkan_result);
		static std::vector<VkExtensionProperties> GetAvailableExtensions();
		static std::vector<VkLayerProperties> GetSupportedValidationLayers();
		static bool CheckIfExtensionSupported(const char* ext);
		static bool CheckIfValidationLayerSupported(const char* layer);
		static inline VkDebugUtilsMessengerCreateInfoEXT CreateDebugMessengerCreateInfo();

		std::vector<VkExtensionProperties> GetSupportedDeviceExtensions() const;

		inline bool CheckDeviceExtensionSupport(std::vector<std::string> deviceExtensions) const;
		inline bool IsDeviceSuitable(VkPhysicalDevice physicalDevice) const;

		inline QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice physicalDevice) const;
		inline VkPhysicalDevice PickPhysicalDevice() const;
		inline VkDevice CreateLogicalDevice();
		inline VkQueue GetDeviceQueue() const;
		inline VkShaderModule CreateShaderModule(const std::vector<char>& code) const;
		inline SwapChainSupportDetails QuerySwapChainSupport() const;
		inline VkQueue GetDevicePresentQueue() const;

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
		void CreateCommandBuffers();
		void CreateSyncObjects();
		void CreateVertexBuffer();
		void CreateDescriptorPool();

		void RecreateSwapChain();
		void CleanupSwapChain() const;

		void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) const;

		static VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		static VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

		void SetupVulkanWindow(VkInstance instance, ImGui_ImplVulkanH_Window* wd, VkSurfaceKHR surface, int width, int height);
		void ImGuiFrameRender(ImGui_ImplVulkanH_Window* wd, ImDrawData* draw_data);
		void ImGuiFramePresent(ImGui_ImplVulkanH_Window* wd);

	private:
		int viewportWidth, viewportHeight;
		uint32_t currentFrame = 0;

		bool framebufferResized = false;


		// Vulkan
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

		std::vector<VkCommandBuffer> commandBuffers;
		std::vector<VkSemaphore> imageAvailableSemaphores;
		std::vector<VkSemaphore> renderFinishedSemaphores;
		std::vector<VkFence> inFlightFences;

		std::vector<VkFramebuffer> swapChainFramebuffers;
		std::vector<VkImageView> swapChainImageViews;

		ImGui_ImplVulkanH_Window* wd;

		VkAllocationCallbacks* g_Allocator = nullptr;
		VkDebugReportCallbackEXT g_DebugReport = VK_NULL_HANDLE;
		VkPipelineCache g_PipelineCache = VK_NULL_HANDLE;
		VkDescriptorPool g_DescriptorPool = VK_NULL_HANDLE;
		ImGui_ImplVulkanH_Window g_MainWindowData;
		uint32_t g_MinImageCount = 2;
	};
}
