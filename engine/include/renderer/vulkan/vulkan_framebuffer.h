#pragma once
#include <vector>

#include "vulkan_image.h"
#include "util/core.h"
#include "vulkan/vulkan.h"

namespace MongooseVK
{
    class VulkanRenderPass;
    class VulkanDevice;

    struct FramebufferAttachment {
        AllocatedImage allocatedImage{};
        VkImageView imageView{};
        VkSampler sampler{};
        VkImageLayout imageLayout{};
        VkFormat format{};
    };

    class VulkanFramebuffer {
    public:
        class Builder {
        public:
            explicit Builder(VulkanDevice* device): device(device) {}
            ~Builder() = default;

            Builder& AddAttachment(VkImageView imageAttachment);
            Builder& AddAttachment(ImageFormat attachmentFormat);
            Builder& SetRenderpass(Ref<VulkanRenderPass> renderPass);
            Builder& SetResolution(uint32_t width, uint32_t height);

            Ref<VulkanFramebuffer> Build();

        private:
            VulkanDevice* device{};
            uint32_t width = 0, height = 0;
            Ref<VulkanRenderPass> renderPass{};
            std::vector<FramebufferAttachment> attachments{};
        };

        struct Params {
            uint32_t width = 0;
            uint32_t height = 0;
            VkFramebuffer framebuffer{};
            std::vector<FramebufferAttachment> attachments{};
            VkRenderPass renderPass{};
        };

    public:
        VulkanFramebuffer(VulkanDevice* _device, const Params& _params): device(_device), params(_params) {}
        ~VulkanFramebuffer();

        VkFramebuffer Get() const { return params.framebuffer; }

        uint32_t GetWidth() const { return params.width; }
        uint32_t GetHeight() const { return params.height; }
        VkExtent2D GetExtent() const { return {params.width, params.height}; }
        std::vector<FramebufferAttachment> const& GetAttachments() const { return params.attachments; }

    private:
        VulkanDevice* device;
        Params params;
    };
}
