#pragma once
#include <vector>
#include <memory/resource_pool.h>

#include "vulkan_image.h"
#include "util/core.h"

namespace MongooseVK
{
    class VulkanRenderPass;
    class VulkanDevice;

    struct FramebufferAttachment {
        AllocatedImage allocatedImage{};
        VkImageView imageView = VK_NULL_HANDLE;
        VkSampler sampler = VK_NULL_HANDLE;
        VkImageLayout imageLayout{};
        VkFormat format = VK_FORMAT_UNDEFINED;
    };

    class VulkanFramebuffer : public PoolObject {
    public:
        class Builder {
        public:
            explicit Builder(VulkanDevice* device): device(device) {}
            ~Builder() = default;

            Builder& AddAttachment(VkImageView imageAttachment);
            Builder& AddAttachment(TextureHandle textureHandle);

            Builder& SetRenderpass(VulkanRenderPass* renderPass);
            Builder& SetResolution(uint32_t width, uint32_t height);

            Ref<VulkanFramebuffer> Build();

        private:
            VulkanDevice* device{};
            uint32_t width = 0, height = 0;
            VulkanRenderPass* renderPass{};
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
