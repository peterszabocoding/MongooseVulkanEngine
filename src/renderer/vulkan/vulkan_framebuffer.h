#pragma once
#include <vector>

#include "vulkan_image.h"
#include "util/core.h"
#include "vulkan/vulkan.h"

namespace Raytracing
{
    class VulkanRenderPass;
    class VulkanDevice;

    class VulkanFramebuffer {
    public:
        class Builder {
        public:
            explicit Builder(VulkanDevice* device): device(device) {}
            ~Builder() = default;

            Builder& AddAttachment(VkImageView imageAttachment);
            Builder& SetRenderpass(Ref<VulkanRenderPass> renderPass);
            Builder& SetResolution(int width, int height);

            Ref<VulkanFramebuffer> Build();

        private:
            VulkanDevice* device{};
            int width = 0, height = 0;
            Ref<VulkanRenderPass> renderPass{};
            std::vector<VkImageView> attachments{};
        };

    public:
        VulkanFramebuffer(VulkanDevice* device, VkFramebuffer framebuffer): device(device), framebuffer(framebuffer) {}
        ~VulkanFramebuffer();

        VkFramebuffer Get() const { return framebuffer; };

    private:
        VulkanDevice* device;
        VkFramebuffer framebuffer;
    };
}
