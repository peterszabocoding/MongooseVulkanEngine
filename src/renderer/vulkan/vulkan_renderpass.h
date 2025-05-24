#pragma once

#include <vector>
#include <glm/vec4.hpp>
#include <vulkan/vulkan_core.h>

#include "util/core.h"

namespace Raytracing
{
    class VulkanFramebuffer;
    class VulkanDevice;

    class VulkanRenderPass {
    public:
        class Builder {
            struct ColorAttachment {
                VkFormat imageFormat;
                VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;
                glm::vec4 clearColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
                bool clearOnLoad = true;
                bool isSwapchainAttachment = false;
            };

            struct DepthAttachment {
                VkFormat depthFormat = VK_FORMAT_D24_UNORM_S8_UINT;
                bool clearOnLoad = true;
            };

        public:
            explicit Builder(VulkanDevice* vulkanDevice);
            Builder& AddColorAttachment(VkFormat imageFormat,
                                        bool isSwapchainAttachment,
                                        bool clearOnLoad = true,
                                        glm::vec4 clearColor = glm::vec4(0.0, 0.0, 0.0, 1.0),
                                        VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT);
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
