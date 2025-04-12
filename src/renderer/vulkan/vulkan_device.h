#pragma once
#include <vector>
#include <vulkan/vulkan_core.h>
#include "vulkan_renderpass.h"

#define GLFW_INCLUDE_VULKAN

#include <vma/vk_mem_alloc.h>

#include "GLFW/glfw3.h"
#include "util/core.h"

namespace Raytracing {
    class VulkanPipeline;
    class VulkanSwapchain;
    class VulkanRenderPass;
    class VulkanMesh;

    constexpr int MAX_FRAMES_IN_FLIGHT = 1;

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    class VulkanDevice {
    public:
        VulkanDevice(int width, int height, GLFWwindow* glfwWindow);

        ~VulkanDevice();

        void DrawMesh(Ref<VulkanPipeline> pipeline, const VulkanMesh* mesh) const;

        void DrawImGui() const;

        bool BeginFrame();

        void EndFrame();

        void ResizeFramebuffer() { framebufferResized = true; }

        VkSurfaceKHR CreateSurface(GLFWwindow* glfwWindow) const;

        VmaAllocator GetVmaAllocator() const { return vmaAllocator; };

        [[nodiscard]] VkInstance GetInstance() const { return instance; }
        [[nodiscard]] VkDevice GetDevice() const { return device; }
        [[nodiscard]] VkSurfaceKHR GetSurface() const { return surface; }
        [[nodiscard]] VkPhysicalDevice GetPhysicalDevice() const { return physicalDevice; }

        [[nodiscard]] uint32_t GetQueueFamilyIndex() const;

        [[nodiscard]] VkDescriptorPool GetDescriptorPool() const { return descriptorPool; }
        [[nodiscard]] VkDescriptorPool GetGuiDescriptorPool() const { return gui_descriptionPool; }
        [[nodiscard]] VkQueue GetGraphicsQueue() const { return graphicsQueue; }
        [[nodiscard]] VkQueue GetPresentQueue() const { return presentQueue; }
        [[nodiscard]] VkRenderPass GetRenderPass() const { return vulkanRenderPass->Get(); }
        [[nodiscard]] VkSemaphore GetImageAvailableSemaphore() const { return imageAvailableSemaphores[currentFrame]; }
        [[nodiscard]] VkSemaphore GetRenderFinishedSemaphore() const { return renderFinishedSemaphores[currentFrame]; }
        [[nodiscard]] VkCommandBuffer GetCurrentCommandBuffer() const { return commandBuffers[currentFrame]; }
        [[nodiscard]] VkCommandPool GetCommandPool() const { return commandPool; }
        [[nodiscard]] Ref<VulkanSwapchain> GetSwapchain() const { return vulkanSwapChain; }

        VkSampleCountFlagBits GetMaxMSAASampleCount() const;

    public:
        [[nodiscard]] inline VkPhysicalDevice PickPhysicalDevice() const;

        inline VkDevice CreateLogicalDevice();

        [[nodiscard]] inline VkQueue GetDeviceQueue() const;

        [[nodiscard]] inline VkQueue GetDevicePresentQueue() const;

    private:
        static VkInstance CreateVkInstance(
            const std::vector<const char *>& deviceExtensions,
            const std::vector<const char *>& validationLayers);


        void Init(int width, int height, GLFWwindow* glfwWindow);

        void CreateCommandPool();

        void CreateCommandBuffers();

        void CreateSyncObjects();

        void CreateGUIDescriptorPool();

        void CreateDescriptorPool();

        bool SetupNextFrame();

        void BeginRenderPass() const;

        void SetViewportAndScissor() const;

        VkResult SubmitDrawCommands(VkSemaphore* signalSemaphores) const;

        VkResult PresentFrame(uint32_t imageIndex, const VkSemaphore* signalSemaphores) const;

    private:
        int viewportWidth{}, viewportHeight{};
        bool framebufferResized = false;
        uint32_t currentFrame = 0;
        uint32_t currentImageIndex = 0;

        VkDevice device{};
        VkInstance instance{};

        VkQueue graphicsQueue{};
        VkQueue presentQueue{};

        VkSurfaceKHR surface{};
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

        VkCommandPool commandPool{};

        std::vector<VkCommandBuffer> commandBuffers;
        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;

        VkDescriptorPool gui_descriptionPool = VK_NULL_HANDLE;
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

        Ref<VulkanSwapchain> vulkanSwapChain{};
        Ref<VulkanRenderPass> vulkanRenderPass{};

        VmaAllocator vmaAllocator;
    };
}
