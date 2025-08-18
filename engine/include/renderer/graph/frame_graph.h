#pragma once

#include <cstdint>
#include <renderer/vulkan/vulkan_device.h>
#include <vulkan/vulkan_core.h>
#include "renderer/scene.h"

namespace MongooseVK
{
    class FrameGraphRenderPass;

    typedef uint32_t FrameGraphHandle;

    struct FrameGraphResourceHandle {
        FrameGraphHandle index;

        bool operator==(const TextureHandle& other) const { return index == other.handle; }
        bool operator!=(const TextureHandle& other) const { return index != other.handle; }
    };

    static FrameGraphResourceHandle INVALID_GRAPH_RESOURCE_HANDLE = {INVALID_RESOURCE_HANDLE};

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

    struct FrameGraphResourceCreate {
        const char* name;
        FrameGraphResourceType type;
        FrameGraphResourceCreateInfo resourceInfo{};
    };


    struct FrameGraphNodeOutput {
        FrameGraphResourceHandle resourceHandle;
        RenderPassOperation::LoadOp loadOp;
        RenderPassOperation::StoreOp storeOp;
    };

    struct FrameGraphNode: PoolObject {
        const char* name = nullptr;

        FrameGraphRenderPass* graphRenderPass;

        std::vector<FrameGraphResourceHandle> inputs;
        std::vector<FrameGraphNodeOutput> outputs;

        std::unordered_map<const char*, FrameGraphNode*> edges;

        bool enabled = true;
        int32_t refCount = 0;
    };

    struct FrameGraphResource: PoolObject{
        const char* name;
        FrameGraphResourceType type;
        FrameGraphResourceCreateInfo resourceInfo{};

        FrameGraphNode* producer = nullptr;
        FrameGraphNode* lastWriter = nullptr;
        uint32_t refCount = 0;
    };

    struct FrameGraphRenderPassOutput {
        FrameGraphResource resource;
        RenderPassOperation::LoadOp loadOp;
        RenderPassOperation::StoreOp storeOp;
    };

    class FrameGraph {
    public:
        FrameGraph(VulkanDevice* device);
        ~FrameGraph() = default;

        void SetResolution(VkExtent2D _resolution);
        void Compile();
        void PreRender(VkCommandBuffer cmd, Scene* scene);
        void Render(VkCommandBuffer cmd, Scene* scene);
        void Resize(VkExtent2D newResolution);

        template<typename T>
        void AddRenderPass(const char* name)
        {
            AddRenderPassImpl(name, new T(device, resolution));
        }

        FrameGraphResourceHandle CreateResource(FrameGraphResourceCreate resourceCreate);
        void CheckResourceExists(const char* resourceName);
        FrameGraphResource* CheckAndGetResourceIfExists(const char* resourceName);

        void WriteResource(const char* resourceName, RenderPassOperation::LoadOp loadOp, RenderPassOperation::StoreOp storeOp);
        void ReadResource(const char* resourceName);
    private:


        FrameGraphResourceHandle CreateTextureResource(const char* resourceName, FrameGraphResourceCreateInfo& createInfo);
        FrameGraphResourceHandle CreateCubeTextureResource(const char* resourceName, FrameGraphResourceCreateInfo& createInfo);
        FrameGraphResourceHandle CreateBufferResource(const char* resourceName, FrameGraphResourceCreateInfo& createInfo);

        void AddRenderPassImpl(const char* name, FrameGraphRenderPass* renderPass);

        void DestroyResources();


    public:
        std::vector<FrameGraphNode*> nodes;
        std::vector<FrameGraphRenderPass*> renderPasses;
        std::unordered_map<std::string, FrameGraphResourceHandle> resourceHandles;

    private:
        VulkanDevice* device;
        VkExtent2D resolution;

        ObjectResourcePool<FrameGraphResource> resourcePool;

        FrameGraphNode* selectedNode = nullptr;
    };
}
