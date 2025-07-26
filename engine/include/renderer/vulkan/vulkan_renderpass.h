#pragma once

#include <array>
#include <glm/vec4.hpp>
#include <memory/resource_pool.h>
#include <resource/resource.h>
#include <vulkan/vulkan_core.h>

namespace MongooseVK
{
    class VulkanFramebuffer;
    class VulkanDevice;

    namespace RenderPassOperation
    {
        enum class LoadOp: uint8_t {
            None = 0,
            Clear,
            Load,
            DontCare
        };

        enum class StoreOp: uint8_t {
            None = 0,
            Store,
            DontCare
        };

        inline VkAttachmentLoadOp convertLoadOpToVk(LoadOp loadOp)
        {
            switch (loadOp)
            {
                case LoadOp::None: return VK_ATTACHMENT_LOAD_OP_NONE;
                case LoadOp::Clear: return VK_ATTACHMENT_LOAD_OP_CLEAR;
                case LoadOp::Load: return VK_ATTACHMENT_LOAD_OP_LOAD;
                case LoadOp::DontCare: return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            }

            return VK_ATTACHMENT_LOAD_OP_NONE;
        }

        inline VkAttachmentStoreOp convertStoreOpToVk(StoreOp storeOp)
        {
            switch (storeOp)
            {
                case StoreOp::None: return VK_ATTACHMENT_STORE_OP_NONE;
                case StoreOp::Store: return VK_ATTACHMENT_STORE_OP_STORE;
                case StoreOp::DontCare: return VK_ATTACHMENT_STORE_OP_DONT_CARE;
            }

            return VK_ATTACHMENT_STORE_OP_NONE;
        }
    }

    class VulkanRenderPass : public PoolObject {
    public:
        struct ColorAttachment {
            ImageFormat imageFormat;
            VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;
            glm::vec4 clearColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
            RenderPassOperation::LoadOp loadOp = RenderPassOperation::LoadOp::Clear;
            RenderPassOperation::StoreOp storeOp = RenderPassOperation::StoreOp::Store;
            VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            VkImageLayout finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            bool isSwapchainAttachment = false;
        };

        struct DepthAttachment {
            ImageFormat depthFormat = ImageFormat::DEPTH24_STENCIL8;
            RenderPassOperation::LoadOp loadOp = RenderPassOperation::LoadOp::Clear;
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

        void Begin(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer, VkExtent2D extent);
        void End(VkCommandBuffer commandBuffer);

        [[nodiscard]] VkRenderPass Get() const { return renderPass; }
        [[nodiscard]] VkRenderPass& Get() { return renderPass; }

    public:
        RenderPassConfig config{};
        VkRenderPass renderPass{};
    };
}
