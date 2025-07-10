#pragma once

#include <array>
#include <glm/vec4.hpp>
#include <memory/resource_pool.h>
#include <vulkan/vulkan_core.h>
#include "util/core.h"

namespace MongooseVK
{
    class VulkanFramebuffer;
    class VulkanDevice;

    class VulkanRenderPass : public PoolObject {
    public:
        struct ColorAttachment {
            VkFormat imageFormat;
            VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;
            glm::vec4 clearColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
            VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            bool isSwapchainAttachment = false;
        };

        struct DepthAttachment {
            VkFormat depthFormat = VK_FORMAT_D24_UNORM_S8_UINT;
            VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        };

        struct RenderPassConfig {
            uint8_t numColorAttachments = 0;
            std::array<ColorAttachment, 8> colorAttachments;
            std::optional<DepthAttachment> depthAttachment;

            void AddColorAttachment(const ColorAttachment& attachment)
            {
                colorAttachments[numColorAttachments++] = attachment;
            }

            void AddDepthAttachment(const DepthAttachment& attachment)
            {
                depthAttachment = attachment;
            }
        };

    public:
        VulkanRenderPass() {}
        ~VulkanRenderPass() = default;

        void Begin(VkCommandBuffer commandBuffer, const Ref<VulkanFramebuffer>& framebuffer, VkExtent2D extent);
        void End(VkCommandBuffer commandBuffer);

        [[nodiscard]] VkRenderPass Get() const { return renderPass; }
        [[nodiscard]] VkRenderPass& Get() { return renderPass; }

    public:
        RenderPassConfig config{};
        VkRenderPass renderPass{};
    };
}
