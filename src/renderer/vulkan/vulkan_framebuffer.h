#pragma once
#include <vector>

#include "vulkan_image.h"
#include "util/core.h"
#include "vulkan/vulkan.h"

namespace Raytracing
{
    class VulkanRenderPass;
    class VulkanDevice;

    struct FramebufferAttachment {
        AllocatedImage allocatedImage;
        VkImageView imageView;
        VkFormat format;
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
            Builder& SetResolution(int width, int height);

            Ref<VulkanFramebuffer> Build();

        private:
            VulkanDevice* device{};
            int width = 0, height = 0;
            Ref<VulkanRenderPass> renderPass{};

            std::vector<FramebufferAttachment> attachments{};
        };

    public:
        VulkanFramebuffer(VulkanDevice* device, const VkFramebuffer framebuffer): device(device), framebuffer(framebuffer) {}

        VulkanFramebuffer(VulkanDevice* _device, const VkFramebuffer _framebuffer, int _width, int _height,
                          const std::vector<FramebufferAttachment>& _attachments): device(_device),
                                                                        framebuffer(_framebuffer),
                                                                        width(_width), height(_height),
                                                                        attachments(_attachments) {}

        ~VulkanFramebuffer();

        VkFramebuffer Get() const { return framebuffer; };

        int GetWidth() const { return width; }
        int GetHeight() const { return height; }
        std::vector<FramebufferAttachment> const& GetAttachments() const { return attachments; };

    private:
        VulkanDevice* device;
        VkFramebuffer framebuffer;

        int width = 0, height = 0;

        std::vector<FramebufferAttachment> attachments{};
    };
}
