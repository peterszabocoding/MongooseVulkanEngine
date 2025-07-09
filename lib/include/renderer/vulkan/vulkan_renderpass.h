#pragma once

#include <vector>
#include <glm/vec4.hpp>
#include <vulkan/vulkan_core.h>

#include "util/core.h"

namespace MongooseVK
{
    class VulkanFramebuffer;
    class VulkanDevice;

    class VulkanRenderPass {
    public:
        struct ColorAttachment {
            VkFormat imageFormat;
            VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;
            glm::vec4 clearColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
            VkAttachmentLoadOp loadOperation = VK_ATTACHMENT_LOAD_OP_CLEAR;
            bool isSwapchainAttachment = false;
        };

        struct DepthAttachment {
            VkFormat depthFormat = VK_FORMAT_D24_UNORM_S8_UINT;
            VkAttachmentLoadOp loadOperation = VK_ATTACHMENT_LOAD_OP_CLEAR;
        };

    public:
        class Builder {
        public:
            explicit Builder(VulkanDevice* vulkanDevice);
            Builder& AddColorAttachment(ColorAttachment colorAttachment);
            Builder& AddDepthAttachment(VkFormat depthFormat = VK_FORMAT_D24_UNORM_S8_UINT);
            Builder& AddDepthAttachment(DepthAttachment depthAttachment);
            Ref<VulkanRenderPass> Build();

        private:
            VulkanDevice* vulkanDevice;

            std::vector<ColorAttachment> colorAttachments;
            std::vector<DepthAttachment> depthAttachments;
            std::vector<VkClearValue> clearValues{};
        };

    public:
        VulkanRenderPass(VulkanDevice* _device, VkRenderPass _renderPass, std::vector<VkClearValue> _clearValues): device(_device),
            renderPass(_renderPass), clearValues(_clearValues) {}

        ~VulkanRenderPass();

        void Begin(VkCommandBuffer commandBuffer, const Ref<VulkanFramebuffer>& framebuffer, VkExtent2D extent);
        void End(VkCommandBuffer commandBuffer);

        [[nodiscard]] VkRenderPass Get() const { return renderPass; }

    private:
        VulkanDevice* device;
        VkRenderPass renderPass{};
        std::vector<VkClearValue> clearValues;
    };
}
