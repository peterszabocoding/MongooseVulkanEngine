#pragma once
#include <vector>

#include "vulkan_image.h"
#include "util/core.h"
#include "vulkan/vulkan.h"

namespace Raytracing
{
    class VulkanRenderPass;
    class VulkanDevice;

    enum class FramebufferAttachmentFormat: uint32_t {
        None = 0,

        // Color
        RGBA8,
        RGB8,
        RGBA16,
        RGBA16F,
        RGBA32F,
        RED_INTEGER,

        // Depth
        DEPTH32,

        // Depth + Stencil
        DEPTH24_STENCIL8
    };


    class VulkanFramebuffer {
    public:
        class Builder {
        public:
            explicit Builder(VulkanDevice* device): device(device) {}
            ~Builder() = default;

            Builder& AddAttachment(VkImageView imageAttachment);
            Builder& AddAttachment(FramebufferAttachmentFormat attachmentFormat);
            Builder& SetRenderpass(Ref<VulkanRenderPass> renderPass);
            Builder& SetResolution(int width, int height);

            Ref<VulkanFramebuffer> Build();

        private:
            VulkanDevice* device{};
            int width = 0, height = 0;
            Ref<VulkanRenderPass> renderPass{};
            std::vector<Ref<VulkanImage>> images{};
            std::vector<Ref<VulkanImageView>> imageViews{};
        };

    public:
        VulkanFramebuffer(VulkanDevice* device, VkFramebuffer framebuffer): device(device), framebuffer(framebuffer) {}

        VulkanFramebuffer(VulkanDevice* _device, VkFramebuffer _framebuffer, std::vector<Ref<VulkanImage>> _images,
                          std::vector<Ref<VulkanImageView>> _imageViews): device(_device), framebuffer(_framebuffer), images(_images),
                                                                 imageViews(_imageViews) {}

        ~VulkanFramebuffer();

        VkFramebuffer Get() const { return framebuffer; };

    private:
        VulkanDevice* device;
        VkFramebuffer framebuffer;

        std::vector<Ref<VulkanImage>> images{};
        std::vector<Ref<VulkanImageView>> imageViews{};
    };
}
