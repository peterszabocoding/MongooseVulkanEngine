#pragma once
#include <vector>
#include <vulkan/vulkan_core.h>
#include "vulkan_renderpass.h"

#define GLFW_INCLUDE_VULKAN

#include <functional>
#include <vma/vk_mem_alloc.h>

#include "vulkan_descriptor_pool.h"
#include "vulkan_mesh.h"
#include "GLFW/glfw3.h"
#include "renderer/camera.h"
#include "util/core.h"

namespace Raytracing
{
    struct SimplePushConstantData;
    class VulkanPipeline;
    class VulkanMesh;

    constexpr int MAX_FRAMES_IN_FLIGHT = 1;

    typedef std::function<void(VkCommandBuffer commandBuffer, uint32_t frameIndex, uint32_t imageIndex)>&& DrawFrameFunction;
    typedef std::function<void()>&& OutOfDateErrorCallback;

    struct DrawCommandParams {
        VkCommandBuffer commandBuffer;
        VulkanMeshlet* meshlet;
        VkPipeline pipeline;
        VkPipelineLayout pipelineLayout;
        std::vector<VkDescriptorSet> descriptorSets{};
        const void* pushConstantData = nullptr;
        uint32_t pushConstantSize = 0;
        VkShaderStageFlags pushConstantShaderStageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    };

    class VulkanDevice {
    public:
        VulkanDevice(int width, int height, GLFWwindow* glfwWindow);
        ~VulkanDevice();

        void DrawMeshlet(const DrawCommandParams& params) const;

        void DrawFrame(VkSwapchainKHR swapchain, VkExtent2D extent, DrawFrameFunction draw, OutOfDateErrorCallback errorCallback);

        void GetReadyToResize();
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

        [[nodiscard]] VkCommandPool GetCommandPool() const { return commandPool; }
        [[nodiscard]] VkPhysicalDeviceProperties GetDeviceProperties() const { return physicalDeviceProperties; }

        VkSampleCountFlagBits GetMaxMSAASampleCount() const;

        void SetViewportAndScissor(VkExtent2D extent, VkCommandBuffer commandBuffer) const;

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

        void CreateCommandPool();
        void CreateCommandBuffers();
        void CreateSyncObjects();
        void CreateDescriptorPool();
        VkResult SetupNextFrame(VkSwapchainKHR swapchain);
        void SetViewportAndScissor(VkExtent2D extent2D) const;


        VkResult SubmitDrawCommands(VkSemaphore* signalSemaphores) const;
        VkResult PresentFrame(VkSwapchainKHR swapchain, uint32_t imageIndex, const VkSemaphore* signalSemaphores) const;

    public:
        uint32_t currentFrame = 0;

    private:
        int viewportWidth{}, viewportHeight{};
        bool getReadyToResize = false;

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

        VmaAllocator vmaAllocator;

        Scope<VulkanDescriptorPool> globalUniformPool{};
        Scope<VulkanDescriptorPool> shaderDescriptorPool{};
        Scope<VulkanDescriptorPool> imguiDescriptorPool{};
    };
}
