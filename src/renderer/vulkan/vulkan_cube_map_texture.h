#pragma once
#include "vulkan_device.h"
#include "util/core.h"

namespace Raytracing
{
    class VulkanCubeMapTexture {
    public:
        class Builder {
        public:
            Builder() = default;
            ~Builder() = default;

            Builder& SetData(void* _data, const uint64_t _size)
            {
                data = _data;
                size = _size;
                return *this;
            }

            Builder& SetResolution(const uint32_t _width, const uint32_t _height)
            {
                width = _width;
                height = _height;
                return *this;
            }

            Builder& SetFormat(const ImageFormat _format)
            {
                format = _format;
                return *this;
            }

            Ref<VulkanCubeMapTexture> Build(VulkanDevice* device);

        private:
            void* data = nullptr;
            uint64_t size = 0;
            uint32_t width = 0;
            uint32_t height = 0;
            ImageFormat format = ImageFormat::Unknown;

            VkImage image{};
            VkImageView imageView{};
            VkDeviceMemory imageMemory{};
            VkSampler sampler{};
            ImageResource imageResource{};
        };

    public:
        VulkanCubeMapTexture(VulkanDevice* _device, const VkImage _image, const VkImageView _imageView, const VkSampler _sampler,
                             const VkDeviceMemory _imageMemory): device(_device),
                                                                 image(_image),
                                                                 imageView(_imageView),
                                                                 imageMemory(_imageMemory),
                                                                 sampler(_sampler) {}

        ~VulkanCubeMapTexture();

        VkImage GetImage() const { return image; };
        VkImageView GetImageView() const { return imageView; }
        VkSampler GetSampler() const { return sampler; }

    private:
        VulkanDevice* device;

        VkImage image{};
        VkImageView imageView{};
        VkDeviceMemory imageMemory{};
        VkSampler sampler{};
    };
}
