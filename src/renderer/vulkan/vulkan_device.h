#pragma once
#include <vector>
#include <vulkan/vulkan_core.h>
#include "vulkan_renderpass.h"

#define GLFW_INCLUDE_VULKAN

#include <functional>
#include <vma/vk_mem_alloc.h>

#include "vulkan_descriptor_pool.h"
#include "vulkan_image_view.h"
#include "GLFW/glfw3.h"
#include "renderer/camera.h"
#include "util/core.h"

namespace Raytracing
{
    struct SimplePushConstantData;
    class VulkanPipeline;
    class VulkanSwapchain;
    class VulkanRenderPass;
    class VulkanMesh;

    constexpr int MAX_FRAMES_IN_FLIGHT = 1;

    class VulkanDevice {
    public:
        VulkanDevice(int width, int height, GLFWwindow* glfwWindow);
        ~VulkanDevice();

        void DrawMesh(
            Ref<Camera> camera,
            const Transform& transform, Ref<VulkanMesh> mesh) const;

        void DrawImGui() const;
        bool BeginFrame();
        void EndFrame();
        void ResizeFramebuffer(int width, int height);
        void ImmediateSubmit(std::function<void (VkCommandBuffer commandBuffer)>&& function) const;

        VkSurfaceKHR CreateSurface(GLFWwindow* glfwWindow) const;
        VmaAllocator GetVmaAllocator() const { return vmaAllocator; };

        [[nodiscard]] VkInstance GetInstance() const { return instance; }
        [[nodiscard]] VkDevice GetDevice() const { return device; }
        [[nodiscard]] VkSurfaceKHR GetSurface() const { return surface; }
        [[nodiscard]] VkPhysicalDevice GetPhysicalDevice() const { return physicalDevice; }
        [[nodiscard]] uint32_t GetQueueFamilyIndex() const;
        [[nodiscard]] VkDescriptorPool GetGuiDescriptorPool() const { return imguiDescriptorPool->GetDescriptorPool(); }
        [[nodiscard]] VulkanDescriptorPool& GetShaderDescriptorPool() const { return *shaderDescriptorPool.get(); }
        [[nodiscard]] VkQueue GetGraphicsQueue() const { return graphicsQueue; }
        [[nodiscard]] VkQueue GetPresentQueue() const { return presentQueue; }
        [[nodiscard]] VkRenderPass GetRenderPass() const { return vulkanRenderPass->Get(); }
        [[nodiscard]] Ref<VulkanRenderPass> GetVulkanRenderPass() const { return vulkanRenderPass; }
        [[nodiscard]] VkCommandPool GetCommandPool() const { return commandPool; }
        [[nodiscard]] VkPhysicalDeviceProperties GetDeviceProperties() const { return physicalDeviceProperties; }
        [[nodiscard]] Ref<VulkanFramebuffer> GetFramebuffer() const { return framebuffers[currentFrame]; }

        VkSampleCountFlagBits GetMaxMSAASampleCount() const;

    public:
        [[nodiscard]] inline VkPhysicalDevice PickPhysicalDevice() const;
        [[nodiscard]] inline VkDevice CreateLogicalDevice();
        [[nodiscard]] inline VkQueue GetDeviceQueue() const;
        [[nodiscard]] inline VkQueue GetDevicePresentQueue() const;

    private:
        static VkInstance CreateVkInstance(
            const std::vector<const char*>& deviceExtensions,
            const std::vector<const char*>& validationLayers);

        void Init(int width, int height, GLFWwindow* glfwWindow);
        void CreateSwapchain();
        void CreateFramebuffers();
        void CreateRenderpass();
        void CreateCommandPool();
        void CreateCommandBuffers();
        void CreateSyncObjects();
        void CreateDescriptorPool();
        bool SetupNextFrame();
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
        VkPhysicalDeviceProperties physicalDeviceProperties;

        VkCommandPool commandPool{};

        std::vector<VkCommandBuffer> commandBuffers;
        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;

        Ref<VulkanRenderPass> vulkanRenderPass{};

        Scope<VulkanSwapchain> vulkanSwapChain{};
        std::vector<Ref<VulkanFramebuffer>> framebuffers;

        VmaAllocator vmaAllocator;

        Scope<VulkanDescriptorPool> globalUniformPool{};
        Scope<VulkanDescriptorPool> shaderDescriptorPool{};
        Scope<VulkanDescriptorPool> imguiDescriptorPool{};
    };
}
