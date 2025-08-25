#pragma once

#include <cstdint>
#include <vulkan/vulkan_core.h>
#include "renderer/scene.h"
#include "renderer/vulkan/vulkan_renderpass.h"
#include "renderer/vulkan/vulkan_device.h"
#include "renderer/vulkan/vulkan_mesh.h"

namespace MongooseVK
{
    namespace FrameGraph
    {
        class FrameGraphRenderPass;

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
        Buffer,
    };

    struct FrameGraphBufferCreateInfo {
        uint64_t size;
        VkBufferUsageFlags usageFlags;
        VmaMemoryUsage memoryUsage;
    };

    struct FrameGraphResourceCreate {
        const char* name;
        FrameGraphResourceType type;
        FrameGraphBufferCreateInfo bufferCreateInfo{};

        TextureHandle textureHandle = INVALID_TEXTURE_HANDLE;
        TextureCreateInfo textureInfo{};
    };

    struct FrameGraphResource: PoolObject{
        const char* name;
        FrameGraphResourceType type;
        AllocatedBuffer allocatedBuffer;

        TextureHandle textureHandle = INVALID_TEXTURE_HANDLE;
        TextureCreateInfo textureInfo{};

        FrameGraphNodeHandle producer;
    };

    struct FrameGraphNodeInputCreate {
        const char* name;
        FrameGraphResourceType type;
        FrameGraphBufferCreateInfo bufferCreateInfo{};

        TextureHandle textureHandle = INVALID_TEXTURE_HANDLE;
        TextureCreateInfo textureInfo{};
    };

    struct FrameGraphNodeInput {
        const char* name;
        FrameGraphResourceType type;
        FrameGraphBufferCreateInfo bufferCreateInfo{};

        TextureHandle textureHandle = INVALID_TEXTURE_HANDLE;
        TextureCreateInfo textureInfo{};
    };

    struct FrameGraphNodeOutputCreate {
        FrameGraphResourceCreate resourceCreate;
        RenderPassOperation::LoadOp loadOp;
        RenderPassOperation::StoreOp storeOp;
    };

    struct FrameGraphNodeOutput {
        FrameGraphResource* resource;
        RenderPassOperation::LoadOp loadOp;
        RenderPassOperation::StoreOp storeOp;
    };

    struct FrameGraphNodeCreation {
        const char* name;
        std::vector<FrameGraphNodeInputCreate> inputs{};
        std::vector<FrameGraphNodeOutputCreate> outputs{};

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

    class FrameGraph {
    public:
        FrameGraph(VulkanDevice* device);
        ~FrameGraph() = default;

        void Init(VkExtent2D _resolution);
        void PreRender(VkCommandBuffer cmd, SceneGraph* scene);
        void Render(VkCommandBuffer cmd, SceneGraph* scene);
        void Resize(VkExtent2D newResolution);
        void Cleanup();

        template<typename T>
        void AddRenderPass(const char* name)
        {
            renderPasses[name] = new T(device, resolution);
            renderPassList.push_back(renderPasses[name]);
        }

        void AddExternalResource(const char* name, FrameGraphResource* resource);
        FrameGraphResource* GetResource(const char* name);


    private:
        FrameGraphResourceHandle CreateTextureResource(const char* resourceName, TextureCreateInfo& createInfo);
        FrameGraphResourceHandle CreateBufferResource(const char* resourceName, FrameGraphBufferCreateInfo& createInfo);

        void DestroyResources();
        void InitializeRenderPasses();
        void CreateFrameGraphOutputs();

        void CreateFrameGraphTextureResource(const char* resourceName, TextureCreateInfo& createInfo);
        void CreateFrameGraphBufferResource(const char* resourceName, FrameGraphBufferCreateInfo& createInfo);

    public:
        std::vector<FrameGraphRenderPass*> renderPassList{};
        std::unordered_map<std::string, FrameGraphRenderPass*> renderPasses;
        std::unordered_map<std::string, FrameGraphResourceHandle> resourceHandles;

        std::unordered_map<std::string, FrameGraphResource*> renderPassResourceMap;
        std::unordered_map<std::string, FrameGraphResource*> externalResources;

    private:
        VulkanDevice* device;
        VkExtent2D resolution;

        ObjectResourcePool<FrameGraphResource> resourcePool;

    };
    }
}
