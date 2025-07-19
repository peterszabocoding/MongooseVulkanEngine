#pragma once

#include <cstdint>
#include <vector>
#include <vulkan/vulkan_core.h>
#include "scene.h"
#include "vulkan/vulkan_renderpass.h"
#include "vulkan/vulkan_device.h"

namespace MongooseVK
{
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
}
