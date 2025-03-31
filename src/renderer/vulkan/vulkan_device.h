#pragma once
#include <vector>
#include <vulkan/vulkan_core.h>
#include "renderer/mesh.h"

#define GLFW_INCLUDE_VULKAN
#include "vulkan_depth_image.h"
#include "GLFW/glfw3.h"

namespace Raytracing
{
	class VulkanPipeline;
	class VulkanVertexBuffer;
	class VulkanIndexBuffer;
	class VulkanTextureImage;
	class VulkanDepthImage;

	constexpr int MAX_FRAMES_IN_FLIGHT = 1;

	struct SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	class VulkanDevice
	{
	public:
		VulkanDevice() = default;
		~VulkanDevice();

		void Init(int width, int height, GLFWwindow* glfwWindow);

		int GetViewportWidth() const { return viewportWidth; }
		int GetViewportHeight() const { return viewportHeight; }

		void Draw();
		void ResizeFramebuffer() { framebufferResized = true; }

		VkInstance GetInstance() const { return instance; }
		VkDevice GetDevice() const { return device; }
		VkSurfaceKHR GetSurface() const { return surface; }
		VkPhysicalDevice GetPhysicalDevice() const { return physicalDevice; }
		uint32_t GetQueueFamilyIndex() const;
		VkDescriptorPool GetDescriptorPool() const { return descriptorPool; }
		VkDescriptorPool GetGuiDescriptorPool() const { return gui_descriptionPool; }
		VkQueue GetGraphicsQueue() const { return graphicsQueue; }
		VkQueue GetPresentQueue() const { return presentQueue; }
		VkRenderPass GetRenderPass() const { return renderPass; }
		VkSemaphore GetImageAvailableSemaphore() const { return imageAvailableSemaphores[currentFrame]; }
		VkSemaphore GetRenderFinishedSemaphore() const { return renderFinishedSemaphores[currentFrame]; }
		VkSwapchainKHR GetCurrentSwapchain() const { return swapChain; }
		VkCommandBuffer GetCurrentCommandBuffer() const { return commandBuffers[currentFrame]; }
		VkSurfaceKHR CreateSurface(GLFWwindow* glfwWindow) const;
		VkCommandPool GetCommandPool() const { return commandPool; }

	public:
		inline VkPhysicalDevice PickPhysicalDevice() const;
		inline VkDevice CreateLogicalDevice();
		inline VkQueue GetDeviceQueue() const;
		inline SwapChainSupportDetails QuerySwapChainSupport() const;
		inline VkQueue GetDevicePresentQueue() const;

	private:
		static VkInstance CreateVkInstance(
			const std::vector<const char*>& deviceExtensions,
			const std::vector<const char*>& validationLayers);

		static void SetViewportAndScissor(VkCommandBuffer commandBuffer, VkExtent2D extent);
		static VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		static VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

		void CreateSwapChain();
		void CreateImageViews();

		VkRenderPass CreateRenderPass(VkDevice device) const;

		void CreateFramebuffers();
		static VkFramebuffer CreateFramebuffer(VkDevice device, VkImageView imageView, const VkImageView depthImageView, VkRenderPass renderPass, uint32_t width,
		                                       uint32_t height);

		void CreateCommandPool();
		void CreateCommandBuffers();
		void CreateSyncObjects();
		void CreateGUIDescriptorPool();
		void CreateDescriptorPool();

		void RecreateSwapChain();
		void CleanupSwapChain() const;
		void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) const;
		void UpdateUniformBuffer() const;
		VkResult SubmitDrawCommands(VkSemaphore* signalSemaphores) const;
		VkResult PresentFrame(uint32_t imageIndex, const VkSemaphore* signalSemaphores) const;

		VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

	private:
		int viewportWidth, viewportHeight;
		uint32_t currentFrame = 0;
		bool framebufferResized = false;

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
		VkCommandPool commandPool;

		std::vector<VkCommandBuffer> commandBuffers;
		std::vector<VkSemaphore> imageAvailableSemaphores;
		std::vector<VkSemaphore> renderFinishedSemaphores;
		std::vector<VkFence> inFlightFences;

		std::vector<VkFramebuffer> swapChainFramebuffers;
		std::vector<VkImageView> swapChainImageViews;

		VkDescriptorPool gui_descriptionPool = VK_NULL_HANDLE;
		VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

		VulkanPipeline* graphicsPipeline;
		VulkanVertexBuffer* vertexBuffer;
		VulkanIndexBuffer* indexBuffer;
		VulkanTextureImage* vulkanImage;
		VulkanDepthImage* vulkanDepthImage;

		std::vector<Vertex> mesh_vertices = Primitives::RECTANGLE_VERTICES;
		std::vector<uint16_t> mesh_indices = Primitives::RECTANGLE_INDICES;
	};
}
