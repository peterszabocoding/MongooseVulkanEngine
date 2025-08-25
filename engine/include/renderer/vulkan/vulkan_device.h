#pragma once
#include <vector>
#include <vulkan/vulkan_core.h>

#define GLFW_INCLUDE_VULKAN

#include <functional>
#include <renderer/bitmap.h>
#include <vma/vk_mem_alloc.h>

#include "GLFW/glfw3.h"

#include "util/core.h"
#include "memory/resource_pool.h"
#include "vulkan_descriptor_pool.h"
#include "vulkan_descriptor_set_layout.h"
#include "vulkan_material.h"
#include "vulkan_pipeline.h"
#include "vulkan_renderpass.h"
#include "resource/resource.h"

namespace MongooseVK
{
    class VulkanRenderPass;
    class VulkanMeshlet;
    class VulkanPipeline;
    class VulkanMesh;
    class VulkanTexture;

    struct SimplePushConstantData;
    struct AllocatedBuffer;

    constexpr int MAX_FRAMES_IN_FLIGHT = 2;
    constexpr int DESCRIPTOR_SET_LAYOUT_POOL_SIZE = 10000;
    constexpr uint32_t MAX_BINDLESS_RESOURCES = 100;
    constexpr uint32_t MAX_OBJECTS = 10000;

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
        PipelineHandle pipelineHandle;
        DrawPushConstantParams pushConstantParams;
        std::vector<VkDescriptorSet> descriptorSets{};
    };

    struct TextureCreateInfo {
        VkExtent2D resolution;
        ImageFormat format;

        void* data = nullptr;
        uint64_t size = 0;

        VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
        VkFilter filter = VK_FILTER_LINEAR;
        VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkBorderColor borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;

        uint32_t mipLevels = 1;
        bool generateMipMaps = false;

        uint8_t arrayLayers = 1;

        bool compareEnabled = false;
        VkCompareOp compareOp = VK_COMPARE_OP_ALWAYS;

        VkImageCreateFlags flags = 0;
        bool isCubeMap = false;
    };

    struct  FramebufferCreationAttachment {
        VkImageView imageView = VK_NULL_HANDLE;
        TextureHandle textureHandle = INVALID_TEXTURE_HANDLE;
    };

    struct FramebufferCreateInfo {
        std::vector<FramebufferCreationAttachment> attachments{};
        RenderPassHandle renderPassHandle = INVALID_RENDER_PASS_HANDLE;
        VkExtent2D resolution{};
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

        void DrawMeshlet(const DrawCommandParams& params);

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

        uint32_t GetDrawCallCount() const;

        // Buffer management
        AllocatedBuffer CreateBuffer(uint64_t size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_AUTO);

        void CopyBuffer(const AllocatedBuffer& src, const AllocatedBuffer& dst);
        void DestroyBuffer(const AllocatedBuffer& buffer);

        // Texture management
        TextureHandle CreateTexture(const TextureCreateInfo& createInfo);
        VulkanTexture* GetTexture(TextureHandle textureHandle);
        void UploadTextureData(TextureHandle textureHandle, void* data, uint64_t size);
        void UploadCubemapTextureData(TextureHandle textureHandle, Bitmap* cubemap);
        void MakeBindlessTexture(TextureHandle textureHandle);
        void DestroyTexture(TextureHandle textureHandle);

        // Material management
        MaterialHandle CreateMaterial(const MaterialCreateInfo& info);
        VulkanMaterial* GetMaterial(MaterialHandle materialHandle);
        void DestroyMaterial(MaterialHandle materialHandle);

        // Render pass management
        RenderPassHandle CreateRenderPass(VulkanRenderPass::RenderPassConfig config);
        void DestroyRenderPass(RenderPassHandle renderPassHandle);

        // Framebuffer management
        FramebufferHandle CreateFramebuffer(FramebufferCreateInfo info);
        VulkanFramebuffer* GetFramebuffer(FramebufferHandle framebufferHandle);
        void DestroyFramebuffer(FramebufferHandle framebufferHandle);

        // Pipeline management
        PipelineHandle CreatePipeline();
        VulkanPipeline* GetPipeline(PipelineHandle pipelineHandle);
        void DestroyPipeline(PipelineHandle pipelineHandle);

        // DescriptorSetLayout management
        DescriptorSetLayoutHandle CreateDescriptorSetLayout(DescriptorSetLayoutCreateInfo& info);
        VulkanDescriptorSetLayout* GetDescriptorSetLayout(DescriptorSetLayoutHandle descriptorSetLayoutHandle);
        void DestroyDescriptorSetLayout(DescriptorSetLayoutHandle descriptorSetLayoutHandle);

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
        ObjectResourcePool<VulkanMaterial> materialPool;
        ObjectResourcePool<VulkanRenderPass> renderPassPool;
        ObjectResourcePool<VulkanFramebuffer> framebufferPool;
        ObjectResourcePool<VulkanPipeline> pipelinePool;
        ObjectResourcePool<VulkanDescriptorSetLayout> descriptorSetLayoutPool;

        VkDescriptorSet bindlessTextureDescriptorSet{};
        DescriptorSetLayoutHandle bindlessTexturesDescriptorSetLayoutHandle;

        VkDescriptorSet materialDescriptorSet{};
        DescriptorSetLayoutHandle materialsDescriptorSetLayoutHandle;
        AllocatedBuffer materialBuffer;

        DeletionQueue frameDeletionQueue;

    private:
        static VulkanDevice* s_Instance;

        uint32_t drawCallCounter = 0;
        uint32_t prevDrawCallCount = 0;

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

        VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    };
}
