#include "renderer/graph/frame_graph.h"

#include <ranges>
#include <renderer/graph/frame_graph_renderpass.h>
#include <renderer/vulkan/vulkan_renderer.h>
#include <renderer/vulkan/vulkan_texture.h>

namespace MongooseVK
{
    FrameGraph::FrameGraph(VulkanDevice* _device): device{_device}
    {
        resourcePool.Init(128);
    }

    void FrameGraph::SetResolution(const VkExtent2D _resolution)
    {
        resolution = _resolution;
    }

    void FrameGraph::Compile()
    {
        for (const auto& node: nodes)
        {
            selectedNode = node;
            selectedNode->graphRenderPass->Setup(this);
        }

        for (const auto& node: nodes)
        {
            for (const auto& input : node->inputs)
            {
                const FrameGraphResource* resource = resourcePool.Get(input.index);
                if (resource->lastWriter)
                {
                    node->edges[resource->lastWriter->name] = resource->lastWriter;
                    continue;
                }

                if (resource->producer)
                {
                    node->edges[resource->producer->name] = resource->producer;
                }
            }
        }
    }

    void FrameGraph::PreRender(const VkCommandBuffer cmd, SceneGraph* scene)
    {
        for (const auto& node: nodes)
        {
            node->graphRenderPass->PreRender(cmd, scene);
        }
    }

    void FrameGraph::Render(const VkCommandBuffer cmd, SceneGraph* scene)
    {
        for (const auto& node: nodes)
        {
            node->graphRenderPass->Render(cmd, scene);
        }
    }

    void FrameGraph::Resize(const VkExtent2D newResolution)
    {
        resolution = newResolution;

        for (const auto& node: nodes)
        {
            node->graphRenderPass->Resize(resolution);
        }
    }

    FrameGraphResourceHandle FrameGraph::CreateResource(FrameGraphResourceCreate resourceCreate)
    {
        FrameGraphResourceHandle resourceHandle = {INVALID_RESOURCE_HANDLE};

        switch (resourceCreate.type)
        {
            case FrameGraphResourceType::Texture:
                resourceHandle = CreateTextureResource(resourceCreate.name, resourceCreate.resourceInfo);
                break;
            case FrameGraphResourceType::TextureCube:
                resourceHandle = CreateCubeTextureResource(resourceCreate.name, resourceCreate.resourceInfo);
                break;
            case FrameGraphResourceType::Buffer:
                resourceHandle = CreateBufferResource(resourceCreate.name, resourceCreate.resourceInfo);
                break;
            default:
                ASSERT(false, "Unknown resource type");
        }
        resourceHandles[resourceCreate.name] = resourceHandle;
        FrameGraphResource* resource = resourcePool.Get(resourceHandle.index);

        if (selectedNode)
        {
            resource->producer = selectedNode;

            FrameGraphNodeOutput output;
            output.loadOp = RenderPassOperation::LoadOp::Clear;
            output.storeOp = RenderPassOperation::StoreOp::Store;
            output.resourceHandle = resourceHandle;

            selectedNode->outputs.push_back(output);
        }

        return resourceHandle;
    }

    void FrameGraph::CheckResourceExists(const char* resourceName) {
        if (!resourceHandles.contains(resourceName))
        {
            LOG_ERROR("Resource not found: {0}", resourceName);
            ASSERT(false, "Resource not found");
            return;
        }

        FrameGraphResource* resource = resourcePool.Get(resourceHandles[resourceName].index);

        if (!resource)
        {
            LOG_ERROR("Resource not found: {0}", resourceName);
            ASSERT(false, "Resource not found");
        }
    }

    FrameGraphResource* FrameGraph::CheckAndGetResourceIfExists(const char* resourceName) {
        if (!resourceHandles.contains(resourceName))
        {
            LOG_ERROR("Resource not found: {0}", resourceName);
            ASSERT(false, "Resource not found");
            return nullptr;
        }

        FrameGraphResource* resource = resourcePool.Get(resourceHandles[resourceName].index);

        if (!resource)
        {
            LOG_ERROR("Resource not found: {0}", resourceName);
            ASSERT(false, "Resource not found");
            return nullptr;
        }

        return resource;
    }

    void FrameGraph::WriteResource(const char* resourceName, RenderPassOperation::LoadOp loadOp, RenderPassOperation::StoreOp storeOp)
    {
        FrameGraphResource* resource = CheckAndGetResourceIfExists(resourceName);

        FrameGraphNodeOutput output;
        output.loadOp = loadOp;
        output.storeOp = storeOp;
        output.resourceHandle = resourceHandles[resourceName];

        selectedNode->outputs.push_back(output);

        if (resource->lastWriter)
        {
            selectedNode->edges[resource->lastWriter->name] = resource->lastWriter;
        }
        resource->lastWriter = selectedNode;
    }

    void FrameGraph::ReadResource(const char* resourceName)
    {
        FrameGraphResource* resource = CheckAndGetResourceIfExists(resourceName);
        selectedNode->inputs.push_back(resourceHandles[resourceName]);
        resource->refCount++;
    }

    FrameGraphResourceHandle FrameGraph::CreateTextureResource(const char* resourceName, FrameGraphResourceCreateInfo& createInfo)
    {
        createInfo.texture.textureHandle = device->CreateTexture(createInfo.texture.textureCreateInfo);

        FrameGraphResource* graphResource = resourcePool.Obtain();
        graphResource->name = resourceName;
        graphResource->type = FrameGraphResourceType::Texture;
        graphResource->resourceInfo = createInfo;
        graphResource->producer = nullptr;
        graphResource->lastWriter = nullptr;
        graphResource->refCount = 0;

        return {graphResource->index};
    }

    FrameGraphResourceHandle FrameGraph::CreateCubeTextureResource(const char* resourceName, FrameGraphResourceCreateInfo& createInfo)
    {
        createInfo.texture.textureHandle = device->CreateTexture(createInfo.texture.textureCreateInfo);

        FrameGraphResource* graphResource = resourcePool.Obtain();
        graphResource->name = resourceName;
        graphResource->type = FrameGraphResourceType::TextureCube;
        graphResource->resourceInfo = createInfo;
        graphResource->producer = nullptr;
        graphResource->lastWriter = nullptr;
        graphResource->refCount = 0;

        return {graphResource->index};
    }

    FrameGraphResourceHandle FrameGraph::CreateBufferResource(const char* resourceName, FrameGraphResourceCreateInfo& createInfo)
    {
        createInfo.buffer.allocatedBuffer = device->CreateBuffer(
            createInfo.buffer.size,
            createInfo.buffer.usageFlags,
            createInfo.buffer.memoryUsage
        );

        FrameGraphResource* graphResource = resourcePool.Obtain();
        graphResource->name = resourceName;
        graphResource->type = FrameGraphResourceType::Buffer;
        graphResource->resourceInfo = createInfo;
        graphResource->producer = nullptr;
        graphResource->lastWriter = nullptr;

        return {graphResource->index};
    }

    void FrameGraph::AddRenderPassImpl(const char* name, FrameGraphRenderPass* renderPass)
    {
        renderPasses.push_back(renderPass);
        FrameGraphNode* node = new FrameGraphNode();

        node->name = name;
        node->enabled = true;
        node->graphRenderPass = renderPass;

        nodes.push_back(node);
    }

    void FrameGraph::DestroyResources()
    {
        resourcePool.ForEach([&](const FrameGraphResource* frameGraphResource) {
            if (frameGraphResource->type == FrameGraphResourceType::Texture)
                device->DestroyTexture(frameGraphResource->resourceInfo.texture.textureHandle);

            if (frameGraphResource->type == FrameGraphResourceType::Buffer)
                device->DestroyBuffer(frameGraphResource->resourceInfo.buffer.allocatedBuffer);
        });

        resourcePool.FreeAllResources();
    }
}
