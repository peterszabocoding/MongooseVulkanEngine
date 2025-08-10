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

    */

    typedef uint32_t FrameGraphHandle;

    struct FrameGraphResourceHandle {
        FrameGraphHandle index;
    };

    struct FrameGraphNodeHandle {
        FrameGraphHandle index;
    };

    enum class FrameGraphResourceType: int8_t {
        Invalid = -1,
        Texture = 0,
        TextureCube,
        Buffer,
    };

    struct FrameGraphResourceCreateInfo {
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

    struct FrameGraphResourceInputCreation {
        const char* name;
        FrameGraphResourceType type;
        FrameGraphResourceCreateInfo resourceInfo{};
    };

    struct FrameGraphResourceOutputCreation {
        const char* name;
        FrameGraphResourceType type;
        FrameGraphResourceCreateInfo resourceInfo{};
    };

    struct FrameGraphResource: PoolObject{
        const char* name;
        FrameGraphResourceType type;
        FrameGraphResourceCreateInfo resourceInfo{};
    };

    struct FrameGraphNodeOutput {
        FrameGraphResource resource;
        RenderPassOperation::LoadOp loadOp;
        RenderPassOperation::StoreOp storeOp;
    };

    struct FrameGraphNodeCreation {
        const char* name;
        std::vector<FrameGraphResourceInputCreation> inputs{};
        std::vector<FrameGraphResourceOutputCreation> outputs{};

        bool enabled = true;
    };

    struct FrameGraphNode {
        const char* name = nullptr;

        FrameGraphRenderPass* graphRenderPass;

        std::vector<FrameGraphResourceHandle> inputs;
        std::vector<FrameGraphResourceHandle> outputs;

        std::vector<FrameGraphNodeHandle> edges;

        bool enabled = true;
        int32_t refCount = 0;
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

        VulkanRenderPass* GetRenderPass() const;

        void AddOutput(const FrameGraphNodeOutput& output);
        void AddInput(const FrameGraphResource& input);

    protected:
        virtual void LoadPipeline(PipelineCreateInfo& pipelineCreate) = 0;

        void CreatePipeline();
        virtual void CreateRenderPass();
        virtual void CreateDescriptors();
        virtual void CreateFramebuffer();

    protected:
        VulkanDevice* device;
        VkExtent2D resolution;

        PipelineHandle pipelineHandle = INVALID_PIPELINE_HANDLE;
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

    private:
        FrameGraphNode* CreateNode(FrameGraphNodeCreation nodeCreation);
        FrameGraphResourceHandle CreateResource(const char* resourceName, FrameGraphResourceType type,
                                                FrameGraphResourceCreateInfo& createInfo);

        FrameGraphResourceHandle CreateTextureResource(const char* resourceName, FrameGraphResourceCreateInfo& createInfo);
        FrameGraphResourceHandle CreateBufferResource(const char* resourceName, FrameGraphResourceCreateInfo& createInfo);

        void DestroyResources();

    public:
        std::unordered_map<std::string, FrameGraphRenderPass*> renderPasses;
        std::unordered_map<std::string, FrameGraphNode*> nodes;
        std::unordered_map<std::string, FrameGraphResourceHandle> resourceHandles;

    private:
        VulkanDevice* device;
        VkExtent2D resolution;

        ObjectResourcePool<FrameGraphResource> resourcePool;

    };
}
