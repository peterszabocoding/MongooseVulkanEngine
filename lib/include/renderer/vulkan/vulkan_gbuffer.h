#pragma once
#include "vulkan_device.h"

namespace MongooseVK
{
    struct BufferAttachment {
        AllocatedImage allocatedImage{};
        VkImageView imageView{};
        VkSampler sampler{};
        VkImageLayout imageLayout{};
        VkFormat format{};
    };

    class VulkanGBuffer {
    public:
        class Builder {
        public:
            Builder() = default;
            ~Builder() = default;

            Builder& SetResolution(uint32_t _width, uint32_t _height);

            Ref<VulkanGBuffer> Build(VulkanDevice* device);

        private:
            BufferAttachment CreateAttachment(VulkanDevice* device, ImageFormat format);

        private:
            uint32_t width, height;
            std::vector<BufferAttachment> attachments;
        };

    public:
        struct Buffers {
            BufferAttachment viewSpaceNormal;
            BufferAttachment viewSpacePosition;
            BufferAttachment depth;
        };

    public:
        VulkanGBuffer(VulkanDevice* device, uint32_t _width, uint32_t _height, const Buffers& _buffers): vulkanDevice(device),
            width(_width), height(_height), buffers(_buffers) {}

        ~VulkanGBuffer();

        uint32_t width, height;
        Buffers buffers;

    private:
        VulkanDevice* vulkanDevice;
    };
}
