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

                BufferHandle buffer;
            } buffer;

            struct {
                uint32_t width;
                uint32_t height;
                uint32_t depth;

                VkFormat format;
                VkImageUsageFlags flags;

                RenderPassOperation::LoadOp loadOp;

                TextureHandle texture;
            } texture;
        };
    };

    struct FrameGraphResource {
        FrameGraphResourceType type;
        FrameGraphResourceInfo resourceInfo;

        FrameGraphNodeHandle producer;
        FrameGraphResourceHandle outputHandle;

        int32_t ref_count = 0;

        const char* name = nullptr;
    };

    struct FrameGraphResourceInputCreation {
        FrameGraphResourceType type;
        FrameGraphResourceInfo resourceInfo;

        const char* name;
    };

    struct FrameGraphResourceOutputCreation {
        FrameGraphResourceType type;
        FrameGraphResourceInfo resourceInfo;

        const char* name;
    };

    struct FrameGraphNodeCreation {
        std::vector<FrameGraphResourceInputCreation> inputs;
        std::vector<FrameGraphResourceOutputCreation> outputs;

        bool enabled;

        const char* name;
    };

    struct FrameGraphRenderPass {
        virtual ~FrameGraphRenderPass() = default;
        virtual void PreRender(VkCommandBuffer cmd, Scene* scene) {}
        virtual void Render(VkCommandBuffer cmd, Scene* scene) {}
        virtual void OnResize(VulkanDevice* device, uint32_t newWidth, uint32_t newHeight) {}
    };

    struct FrameGraphNode {
        int32_t ref_count = 0;

        RenderPassHandle renderPass;
        FramebufferHandle framebuffer;

        FrameGraphRenderPass* graphRenderPass;

        std::vector<FrameGraphResourceHandle> inputs;
        std::vector<FrameGraphResourceHandle> outputs;

        std::vector<FrameGraphNodeHandle> edges;

        bool enabled = true;

        const char* name = nullptr;
    };
}
