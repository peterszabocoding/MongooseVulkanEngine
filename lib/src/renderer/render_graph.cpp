#include "renderer/render_graph.h"
#include "renderer/vulkan/vulkan_device.h"

namespace MongooseVK
{
    RenderResource::~RenderResource() {}

    void RenderResource::Destroy(const VkDevice device)
    {
        if (imageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(device, imageView, nullptr);
            imageView = VK_NULL_HANDLE;
        }

        if (image != VK_NULL_HANDLE)
        {
            vkDestroyImage(device, image, nullptr);
            image = VK_NULL_HANDLE;
        }

        if (memory != VK_NULL_HANDLE)
        {
            vkFreeMemory(device, memory, nullptr);
            memory = VK_NULL_HANDLE;
        }
    }

    void RenderNode::AddResourceAccess(const std::string& resourceName, ResourceAccess access)
    {
        resourceAccesses.emplace_back(resourceName, access);
    }

    RenderGraph::RenderGraph(VulkanDevice* device) : device(device) {}

    RenderGraph::~RenderGraph()
    {
        for (auto& [name, resource]: resources)
        {
            resource->Destroy(device->GetDevice());
        }
    }

    RenderResource* RenderGraph::AddResource(const std::string& name, const VkFormat format, const VkExtent2D extent)
    {
        auto resource = CreateScope<RenderResource>();
        resource->name = name;
        resource->format = format;
        resource->extent = extent;

        // Allocate image
        resource->allocatedImage = ImageBuilder(device)
                .SetFormat(format)
                .SetResolution(extent.width, extent.height)
                .SetTiling(VK_IMAGE_TILING_OPTIMAL)
                .AddUsage(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
                .AddUsage(VK_IMAGE_USAGE_SAMPLED_BIT)
                .SetInitialLayout(VK_IMAGE_LAYOUT_UNDEFINED)
                .SetMipLevels(1)
                .SetArrayLayers(1)
                .Build();

        // Create image view
        resource->imageView = ImageViewBuilder(device)
                .SetFormat(format)
                .SetImage(resource->allocatedImage.image)
                .SetViewType(VK_IMAGE_VIEW_TYPE_2D)
                .SetAspectFlags(VK_IMAGE_ASPECT_COLOR_BIT)
                .SetMipLevels(1)
                .SetBaseArrayLayer(0)
                .Build();

        resource->currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        resources[name] = std::move(resource);
        return resources[name].get();
    }

    RenderNode* RenderGraph::AddNode(const std::string& name)
    {
        auto node = CreateScope<RenderNode>();
        node->name = name;

        nodes.push_back(std::move(node));
        return nodes.back().get();
    }

    void RenderGraph::Compile()
    {
        // Build a simple dependency graph for now
        std::unordered_map<std::string, std::set<std::string>> resourceDependencies;
        std::unordered_map<std::string, std::set<std::string>> nodeDependencies;

        // Track which nodes write to each resource
        std::unordered_map<std::string, std::string> lastWriter;

        // First pass: determine dependencies based on resource access
        for (const auto& node: nodes)
        {
            for (const auto& [resourceName, access]: node->resourceAccesses)
            {
                // If we're reading a resource that someone wrote to, we depend on that node
                if (access == ResourceAccess::Read || access == ResourceAccess::ReadWrite)
                {
                    if (lastWriter.contains(resourceName))
                    {
                        nodeDependencies[node->name].insert(lastWriter[resourceName]);
                    }
                }

                // If we're writing, update the last writer
                if (access == ResourceAccess::Write || access == ResourceAccess::ReadWrite)
                {
                    lastWriter[resourceName] = node->name;
                }
            }
        }

        // Topological sort to determine execution order
        std::unordered_map<std::string, bool> visited;
        std::unordered_map<std::string, bool> inStack;

        std::function<void(const std::string&)> topoSort = [&](const std::string& nodeName) {
            visited[nodeName] = true;
            inStack[nodeName] = true;

            if (nodeDependencies.contains(nodeName))
            {
                for (const auto& dep: nodeDependencies[nodeName])
                {
                    if (!visited[dep])
                    {
                        topoSort(dep);
                    } else if (inStack[dep])
                    {
                        throw std::runtime_error("Cycle detected in render graph");
                    }
                }
            }

            inStack[nodeName] = false;

            // Find the node with this name
            for (const auto& node: nodes)
            {
                if (node->name == nodeName)
                {
                    executionOrder.push_back(node.get());
                    break;
                }
            }
        };

        // Start topo sort from each unvisited node
        for (const auto& node: nodes)
        {
            if (!visited[node->name])
            {
                topoSort(node->name);
            }
        }

        // Reverse to get correct execution order
        std::reverse(executionOrder.begin(), executionOrder.end());

        // Plan resource transitions
        PlanResourceTransitions();
    }

    void RenderGraph::PlanResourceTransitions()
    {
        // For each node in execution order, determine the required resource layouts
        for (const auto& node: executionOrder)
        {
            for (const auto& [resourceName, access]: node->resourceAccesses)
            {
                auto resourceIt = resources.find(resourceName);
                if (resourceIt == resources.end())
                {
                    throw std::runtime_error("Resource not found: " + resourceName);
                }

                auto* resource = resourceIt->second.get();

                // Determine required layout based on access type
                if (access == ResourceAccess::Read)
                {
                    resource->requiredLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    resource->srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                    resource->dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                    resource->srcAccess = 0;
                    resource->dstAccess = VK_ACCESS_SHADER_READ_BIT;
                } else if (access == ResourceAccess::Write)
                {
                    resource->requiredLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                    resource->srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                    resource->dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                    resource->srcAccess = 0;
                    resource->dstAccess = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                } else if (access == ResourceAccess::ReadWrite)
                {
                    resource->requiredLayout = VK_IMAGE_LAYOUT_GENERAL;
                    resource->srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                    resource->dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                    resource->srcAccess = 0;
                    resource->dstAccess = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
                }
            }
        }
    }

    void RenderGraph::InsertBarriers(const VkCommandBuffer commandBuffer,
                                     RenderResource* resource,
                                     const VkImageLayout newLayout,
                                     const VkPipelineStageFlags srcStage,
                                     const VkPipelineStageFlags dstStage,
                                     const VkAccessFlags srcAccess,
                                     const VkAccessFlags dstAccess)
    {
        if (resource->currentLayout == newLayout)
        {
            return;
        }

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = resource->currentLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = resource->image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = srcAccess;
        barrier.dstAccessMask = dstAccess;

        vkCmdPipelineBarrier(
            commandBuffer,
            srcStage,
            dstStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        resource->currentLayout = newLayout;
    }

    void RenderGraph::Execute(const VkCommandBuffer commandBuffer)
    {
        // For each node in the execution order
        for (const auto& node: executionOrder)
        {
            // Create a map of resources this node uses
            std::unordered_map<std::string, RenderResource*> nodeResources;

            // First, handle all necessary transitions
            for (const auto& [resourceName, access]: node->resourceAccesses)
            {
                auto* resource = resources[resourceName].get();
                nodeResources[resourceName] = resource;

                // Insert image memory barrier if layout needs to change
                InsertBarriers(
                    commandBuffer,
                    resource,
                    resource->requiredLayout,
                    resource->srcStage,
                    resource->dstStage,
                    resource->srcAccess,
                    resource->dstAccess
                );
            }

            // Execute the node's function
            if (node->executeFunc)
            {
                node->executeFunc(commandBuffer, nodeResources);
            }
        }
    }
}
