#pragma once
#include "vulkan_device.h"
#include "renderer/bitmap.h"
#include "util/core.h"

namespace Raytracing
{
    class VulkanCubeMapTexture {
    public:
        class Builder {
        public:
            Builder() = default;
            ~Builder() = default;

            Builder& SetData(Bitmap* _bitmap)
            {
                cubemap = _bitmap;
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
            Bitmap* cubemap = nullptr;
            uint32_t width = 0;
            uint32_t height = 0;
            ImageFormat format = ImageFormat::Unknown;

            AllocatedImage allocatedImage{};
            VkImageView imageView{};
            std::array<VkImageView, 6> faceImageViews{};
            VkDeviceMemory imageMemory{};
            VkSampler sampler{};
            ImageResource imageResource{};
        };

    public:
        VulkanCubeMapTexture(VulkanDevice* _device, const AllocatedImage _image, const VkImageView _imageView,
                             const std::array<VkImageView, 6> _faceImageViews, const VkSampler _sampler,
                             const VkDeviceMemory _imageMemory): device(_device),
                                                                 allocatedImage(_image),
                                                                 imageView(_imageView),
                                                                 faceImageViews(_faceImageViews),
                                                                 imageMemory(_imageMemory),
                                                                 sampler(_sampler) {}

        ~VulkanCubeMapTexture();

        VkImage GetImage() const { return allocatedImage.image; };
        VkImageView GetImageView() const { return imageView; }
        VkImageView GetFaceImageView(size_t faceIndex) const { return faceImageViews[faceIndex]; }
        VkSampler GetSampler() const { return sampler; }

    private:
        VulkanDevice* device;

        AllocatedImage allocatedImage{};
        VkImageView imageView{};
        std::array<VkImageView, 6> faceImageViews{};
        VkDeviceMemory imageMemory{};
        VkSampler sampler{};
    };
}
