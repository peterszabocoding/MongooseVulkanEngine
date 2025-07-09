#pragma once
#include <vector>
#include <vulkan/vulkan_core.h>

#define GLFW_INCLUDE_VULKAN

#include <functional>
#include <vma/vk_mem_alloc.h>

#include "GLFW/glfw3.h"

#include "util/core.h"
#include "memory/resource_pool.h"
#include "vulkan_descriptor_pool.h"
#include "vulkan_descriptor_set_layout.h"
#include "vulkan_texture.h"
#include "resource/resource.h"

namespace MongooseVK
{
    class VulkanMeshlet;
    class VulkanPipeline;
    class VulkanMesh;

    struct SimplePushConstantData;
    struct AllocatedBuffer;

    constexpr int MAX_FRAMES_IN_FLIGHT = 1;
    constexpr int DESCRIPTOR_SET_LAYOUT_POOL_SIZE = 10000;
    constexpr uint32_t MAX_BINDLESS_RESOURCES = 100;

    typedef std::function<void(VkCommandBuffer commandBuffer, uint32_t imageIndex)>&& DrawFrameFunction;
    typedef std::function<void()>&& OutOfDateErrorCallback;

    struct DrawPipelineParams {
        VkPipeline pipeline;
        VkPipelineLayout pipelineLayout;
    };

    struct DrawPushConstantParams {
        void* data;
        uint32_t size;
        VkShaderStageFlags shaderStageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    };

    struct DrawCommandParams {
        VkCommandBuffer commandBuffer;
        VulkanMeshlet* meshlet;
        DrawPipelineParams pipelineParams;
        DrawPushConstantParams pushConstantParams;
        std::vector<VkDescriptorSet> descriptorSets{};
    };

    struct AllocatedBuffer {
        VkBuffer buffer;
        VmaAllocation allocation;
        VmaAllocationInfo info;
        VkDeviceAddress address;

        VkDeviceMemory GetBufferMemory() const { return info.deviceMemory; }
        VkDeviceSize GetBufferSize() const { return info.size; }
        VkDeviceSize GetOffset() const { return info.offset; }
        void* GetData() const { return info.pMappedData; }
    };

    class DeletionQueue {
    public:
        void Push(const std::function<void()>& func)
        {
            deletions.push_back(func);
        }

        void Flush()
        {
            for (auto& func: deletions) func();
            deletions.clear();
        }

    private:
        std::vector<std::function<void()>> deletions;
    };

    class VulkanDevice {
    public:
        VulkanDevice(GLFWwindow* glfwWindow);
        ~VulkanDevice();

        static VulkanDevice* Create(GLFWwindow* glfwWindow);
        static VulkanDevice* Get() { return s_Instance; }

        void DrawMeshlet(const DrawCommandParams& params) const;

        void DrawFrame(VkSwapchainKHR swapchain, DrawFrameFunction draw, OutOfDateErrorCallback errorCallback);

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

        void SetViewportAndScissor(VkExtent2D extent, VkCommandBuffer commandBuffer) const;

        [[nodiscard]] inline VkPhysicalDevice PickPhysicalDevice() const;
        [[nodiscard]] inline VkDevice CreateLogicalDevice();
        [[nodiscard]] inline VkQueue GetDeviceQueue() const;
        [[nodiscard]] inline VkQueue GetDevicePresentQueue() const;

        // Buffer management
        AllocatedBuffer AllocateBuffer(uint64_t size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_AUTO);
        void CopyBuffer(const AllocatedBuffer& src, const AllocatedBuffer& dst);
        void FreeBuffer(AllocatedBuffer buffer);

        // Texture management
        TextureHandle AllocateTexture(ImageResource imageResource);
        void UpdateTexture(TextureHandle textureHandle);
        void FreeTexture(TextureHandle textureHandle);

    private:
        static VkInstance CreateVkInstance(
            const std::vector<const char*>& deviceExtensions,
            const std::vector<const char*>& validationLayers);

        void Init(GLFWwindow* glfwWindow);

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
        ObjectResourcePool<VulkanTexture> texturePool;
        VkDescriptorSet bindlessTextureDescriptorSet{};
        Ref<VulkanDescriptorSetLayout> bindlessDescriptorSetLayout;

        DeletionQueue frameDeletionQueue;

    private:
        static VulkanDevice* s_Instance;

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

        Scope<VulkanDescriptorPool> bindlessDescriptorPool{};

        ObjectResourcePool<VulkanDescriptorSetLayout> descriptorSetLayoutPool{};

        VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    };
}
