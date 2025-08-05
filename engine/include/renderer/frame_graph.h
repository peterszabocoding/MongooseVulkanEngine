#pragma once

#include <cstdint>
#include <vulkan/vulkan_core.h>
#include "scene.h"
#include "vulkan/vulkan_renderpass.h"
#include "vulkan/vulkan_device.h"

namespace MongooseVK
{
    class FrameGraphRenderPass;
    /*
    typedef uint32_t FrameGraphHandle;

    struct FrameGraphResourceHandle {
        FrameGraphHandle index;
    };

    struct FrameGraphNodeHandle {
        FrameGraphHandle index;
    };

    enum FrameGraphResourceType {
        Invalid = -1,
        Buffer = 0,
        Texture = 1,
        Attachment = 2,
        Reference = 3
    };

    struct FrameGraphResourceInfo {
        bool external = false;

        union {
            struct {
                size_t size;
                VkBufferUsageFlags flags;
                VmaMemoryUsage memoryUsage;

                AllocatedBuffer buffer;
            } buffer;

            struct {
                uint32_t width;
                uint32_t height;
                uint32_t depth;

                ImageFormat format;
                VkImageUsageFlags flags;

                RenderPassOperation::LoadOp loadOp;

                TextureHandle texture;
            } texture;
        };
    };

    struct FrameGraphResource {
        const char* name = nullptr;

        FrameGraphResourceType type;
        FrameGraphResourceInfo resourceInfo;

        FrameGraphNodeHandle producer;
        FrameGraphResourceHandle outputHandle;

        int32_t ref_count = 0;
    };

    struct FrameGraphResourceInputCreation {
        const char* name;
        FrameGraphResourceType type;
        FrameGraphResourceInfo resourceInfo;
    };

    struct FrameGraphResourceOutputCreation {
        const char* name;
        FrameGraphResourceType type;
        FrameGraphResourceInfo resourceInfo;
    };

    struct FrameGraphNodeCreation {
        const char* name;
        std::vector<FrameGraphResourceInputCreation> inputs{};
        std::vector<FrameGraphResourceOutputCreation> outputs{};

        bool enabled = true;
    };

    class FrameGraphRenderPass {
    public:
        virtual ~FrameGraphRenderPass() = default;
        virtual void PreRender(VkCommandBuffer cmd, Scene* scene) {}
        virtual void Render(VkCommandBuffer cmd, Scene* scene) {}
        virtual void OnResize(VulkanDevice* device, uint32_t newWidth, uint32_t newHeight) {}
    };

    struct FrameGraphNode {
        const char* name = nullptr;

        RenderPassHandle renderPass;
        FramebufferHandle framebuffer;

        FrameGraphRenderPass* graphRenderPass;

        std::vector<FrameGraphResourceHandle> inputs;
        std::vector<FrameGraphResourceHandle> outputs;

        std::vector<FrameGraphNodeHandle> edges;

        bool enabled = true;
        int32_t refCount = 0;
    };

    */

    enum class FrameGraphResourceType: int8_t {
        Invalid = -1,
        Texture = 0,
        TextureCube,
        Buffer,
    };

    struct FrameGraphResourceInfo {
        union {
            struct {
                uint64_t size;
                VkBufferUsageFlags usageFlags;
                VmaMemoryUsage memoryUsage;
                AllocatedBuffer allocatedBuffer;
            } buffer;

            struct {
                TextureHandle textureHandle;
                TextureCreateInfo textureCreateInfo;
            } texture;
        };
    };

    struct FrameGraphResource {
        std::string name;
        FrameGraphResourceType type;
        FrameGraphResourceInfo resourceInfo{};
    };

    struct FrameGraphNodeOutput {
        FrameGraphResource resource;
        RenderPassOperation::LoadOp loadOp;
        RenderPassOperation::StoreOp storeOp;
    };

    class FrameGraphRenderPass {
    public:
        FrameGraphRenderPass(VulkanDevice* vulkanDevice, VkExtent2D _resolution): device(vulkanDevice), resolution(_resolution) {}
        virtual ~FrameGraphRenderPass() { FrameGraphRenderPass::Reset(); }

        virtual void Init();
        virtual void Reset();
        virtual void Resize(VkExtent2D _resolution);

        virtual void PreRender(VkCommandBuffer commandBuffer, Scene* scene) {}
        virtual void Render(VkCommandBuffer commandBuffer, Scene* scene) {}
        virtual void OnResolutionChanged(const uint32_t width, const uint32_t height) {}

        virtual VulkanRenderPass* GetRenderPass() const;
        virtual RenderPassHandle GetRenderPassHandle() const;

        void AddOutput(const FrameGraphNodeOutput& output);
        void AddInput(const FrameGraphResource& input);

    protected:
        virtual void LoadPipeline() = 0;

        virtual void CreateRenderPass();
        virtual void CreateDescriptors();
        virtual void CreateFramebuffer();

    protected:
        VulkanDevice* device;
        VkExtent2D resolution;

        PipelineCreate pipelineConfig{};
        VulkanPipeline* pipeline = nullptr;
        PipelineHandle pipelineHandle = INVALID_PIPELINE_HANDLE;

        VulkanRenderPass::RenderPassConfig renderpassConfig{};
        RenderPassHandle renderPassHandle = INVALID_RENDER_PASS_HANDLE;

        std::vector<FramebufferHandle> framebufferHandles;

        DescriptorSetLayoutHandle passDescriptorSetLayoutHandle = INVALID_DESCRIPTOR_SET_LAYOUT_HANDLE;
        VkDescriptorSet passDescriptorSet = VK_NULL_HANDLE;

        std::vector<FrameGraphResource> inputs;
        std::vector<FrameGraphNodeOutput> outputs;
    };

    class FrameGraph {
    public:
        FrameGraph(VulkanDevice* device);
        ~FrameGraph() = default;

        void Init(VkExtent2D _resolution);
        void PreRender(VkCommandBuffer cmd, Scene* scene);
        void Render(VkCommandBuffer cmd, Scene* scene);
        void Resize(VkExtent2D newResolution);

        void AddRenderPass(const std::string& name, FrameGraphRenderPass* frameGraphRenderPass);
        void AddResource(FrameGraphResource* frameGraphResouce);

    public:
        std::unordered_map<std::string, FrameGraphRenderPass*> frameGraphRenderPasses;
        std::unordered_map<std::string, FrameGraphResource*> frameGraphResources;

    private:
        VulkanDevice* device;
        VkExtent2D resolution;
    };
}
