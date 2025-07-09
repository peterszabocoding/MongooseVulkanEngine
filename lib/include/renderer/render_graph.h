#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <vulkan/vulkan_core.h>

#include "vulkan/vulkan_image.h"

namespace MongooseVK
{
    class VulkanDevice;

    // Represents access type to a resource
    enum class ResourceAccess {
        None,
        Read,
        Write,
        ReadWrite
    };

    // Represents a render resource (texture, buffer, etc.)
    class RenderResource {
    public:
        ~RenderResource();
        void Destroy(VkDevice device);

    public:
        std::string name;
        VkFormat format = VK_FORMAT_UNDEFINED;
        VkExtent2D extent = {0, 0};
        AllocatedImage allocatedImage;
        VkImage image = VK_NULL_HANDLE;
        VkImageView imageView = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkImageLayout currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        // For transition planning
        VkImageLayout requiredLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        VkPipelineStageFlags srcStage = 0;
        VkPipelineStageFlags dstStage = 0;
        VkAccessFlags srcAccess = 0;
        VkAccessFlags dstAccess = 0;
    };

    // Represents a node in the render graph
    class RenderNode {
    public:
        void AddResourceAccess(const std::string& resourceName, ResourceAccess access);

    public:
        std::string name;
        std::function<void(VkCommandBuffer, const std::unordered_map<std::string, RenderResource*>&)> executeFunc;
        std::vector<std::pair<std::string, ResourceAccess>> resourceAccesses;
    };

    // Main render graph class
    class RenderGraph {
    public:
        explicit RenderGraph(VulkanDevice* device);
        ~RenderGraph();

        // Add a render resource to the graph
        RenderResource* AddResource(const std::string& name, VkFormat format, VkExtent2D extent);

        // Add a render node/pass to the graph
        RenderNode* AddNode(const std::string& name);

        // Compile the graph - analyzes dependencies and creates execution plan
        void Compile();

        // Execute the compiled graph
        void Execute(VkCommandBuffer commandBuffer);

    private:
        void InsertBarriers(VkCommandBuffer commandBuffer,
                            RenderResource* resource,
                            VkImageLayout newLayout,
                            VkPipelineStageFlags srcStage,
                            VkPipelineStageFlags dstStage,
                            VkAccessFlags srcAccess,
                            VkAccessFlags dstAccess);

        void PlanResourceTransitions();

    private:
        VulkanDevice* device;
        std::unordered_map<std::string, Scope<RenderResource>> resources;
        std::vector<Scope<RenderNode>> nodes;
        std::vector<RenderNode*> executionOrder;
    };

}
