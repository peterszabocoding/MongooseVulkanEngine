#pragma once

#define GLFW_INCLUDE_VULKAN

#include <optional>

#include "vulkan/vulkan.h"
#include "renderer/mesh.h"
#include "renderer/renderer.h"

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
		~VulkanRenderer() override;

		virtual void Init(int width, int height) override;

		virtual void ProcessPixel(unsigned int pixelCount, vec3 pixelColor) override {}
		virtual void OnRenderBegin(const Camera& camera) override {}
		virtual void OnRenderFinished(const Camera& camera) override {}

		virtual void IdleWait() override;
		virtual void Resize(int width, int height) override;
		virtual void DrawFrame() override;

		int GetViewportWidth() const { return viewportWidth; }
		int GetViewportHeight() const { return viewportHeight; }

		VkInstance GetInstance() const { return instance; }
		VkDevice GetDevice() const { return device; }
		VkSurfaceKHR GetSurface() const { return surface; }
		VkPhysicalDevice GetPhysicalDevice() const { return physicalDevice; }
		uint32_t GetQueueFamilyIndex() const;
		VkDescriptorPool GetDescriptorPool() const { return g_DescriptorPool; }
		VkQueue GetGraphicsQueue() const { return graphicsQueue; }
		VkQueue GetPresentQueue() const { return presentQueue; }
		VkRenderPass GetRenderPass() const { return renderPass; }

		VkAllocationCallbacks* GetAllocationCallbackPointer() const { return g_Allocator; }

		VkSemaphore GetImageAvailableSemaphore() const { return imageAvailableSemaphores[currentFrame]; }
		VkSemaphore GetRenderFinishedSemaphore() const { return renderFinishedSemaphores[currentFrame]; }

		VkSwapchainKHR GetCurrentSwapchain() const { return swapChain; }

		VkCommandBuffer GetCurrentCommandBuffer() { return commandBuffers[currentFrame]; }

		VkSurfaceKHR CreateSurface() const;

	public:
		inline VkPhysicalDevice PickPhysicalDevice() const;
		inline VkDevice CreateLogicalDevice();
		inline VkQueue GetDeviceQueue() const;
		inline VkShaderModule CreateShaderModule(const std::vector<char>& code) const;
		inline SwapChainSupportDetails QuerySwapChainSupport() const;
		inline VkQueue GetDevicePresentQueue() const;

	private:
		void InitVulkan();

		static VkInstance CreateVkInstance(
			const std::vector<const char*>& deviceExtensions,
			const std::vector<const char*>& validationLayers);

		void CreateSwapChain();
		void CreateImageViews();

		VkImageView CreateImageView(VkImage image) const;

		VkPipeline CreateGraphicsPipeline(VkDevice device);
		VkRenderPass CreateRenderPass(VkDevice device) const;

		void CreateFramebuffers();
		static VkFramebuffer CreateFramebuffer(VkDevice device, VkImageView imageView, VkRenderPass renderPass, uint32_t width,
		                                       uint32_t height);

		void CreateCommandPool();
		void CreateCommandBuffers();
		void CreateSyncObjects();
		void CreateVertexBuffer(VkDevice device, const std::vector<Vertex>& vertexData);
		void CreateIndexBuffer(const VkDevice device, const std::vector<uint16_t> mesh_indices);

		void CreateDescriptorPool();
		void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer,
		                  VkDeviceMemory& bufferMemory) const;

		void RecreateSwapChain();
		void CleanupSwapChain() const;

		void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) const;

		void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const;

		static VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		static VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

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

		VkDescriptorPool g_DescriptorPool = VK_NULL_HANDLE;
		VkAllocationCallbacks* g_Allocator = nullptr;

		VkBuffer vertexBuffer;
		VkDeviceMemory vertexBufferMemory;

		VkBuffer indexBuffer;
		VkDeviceMemory indexBufferMemory;

		std::vector<Vertex> mesh_vertices = Primitives::RECTANGLE_VERTICES;
		std::vector<uint16_t> mesh_indices = Primitives::RECTANGLE_INDICES;
	};
}
