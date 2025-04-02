#pragma once
#include <vector>
#include <vulkan/vulkan_core.h>

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

namespace Raytracing
{
    class VulkanPipeline;
    class VulkanSwapchain;
    class Mesh;

    constexpr int MAX_FRAMES_IN_FLIGHT = 1;

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    class VulkanDevice {
    public:
        VulkanDevice() = default;
        ~VulkanDevice();

        void Init(int width, int height, GLFWwindow* glfwWindow);

        void Draw(VulkanPipeline* pipeline, Mesh* mesh);
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
        VkCommandBuffer GetCurrentCommandBuffer() const { return commandBuffers[currentFrame]; }
        VkSurfaceKHR CreateSurface(GLFWwindow* glfwWindow) const;
        VkCommandPool GetCommandPool() const { return commandPool; }
        VulkanSwapchain* GetSwapchain() { return vulkanSwapChain; }

    public:
        inline VkPhysicalDevice PickPhysicalDevice() const;
        inline VkDevice CreateLogicalDevice();
        inline VkQueue GetDeviceQueue() const;
        inline VkQueue GetDevicePresentQueue() const;

    private:
        static VkInstance CreateVkInstance(
            const std::vector<const char*>& deviceExtensions,
            const std::vector<const char*>& validationLayers);

        static void SetViewportAndScissor(VkCommandBuffer commandBuffer, VkExtent2D extent);
        VkRenderPass CreateRenderPass(VkDevice device) const;

        void CreateCommandPool();
        void CreateCommandBuffers();
        void CreateSyncObjects();
        void CreateGUIDescriptorPool();
        void CreateDescriptorPool();

        void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, VulkanPipeline* pipeline, Mesh* mesh) const;
        VkResult SubmitDrawCommands(VkSemaphore* signalSemaphores) const;
        VkResult PresentFrame(uint32_t imageIndex, const VkSemaphore* signalSemaphores) const;

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

        VkRenderPass renderPass;
        VkCommandPool commandPool;

        std::vector<VkCommandBuffer> commandBuffers;
        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;

        VkDescriptorPool gui_descriptionPool = VK_NULL_HANDLE;
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

        VulkanSwapchain* vulkanSwapChain;
    };
}
