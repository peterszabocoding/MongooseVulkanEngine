#pragma once
#include "vulkan_device.h"
#include "vulkan_image.h"
#include "renderer/bitmap.h"
#include "util/core.h"

namespace MongooseVK
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

            Builder& SetMipLevels(uint32_t _miplevels)
            {
                mipLevels = _miplevels;
                return *this;
            }

            Ref<VulkanCubeMapTexture> Build(VulkanDevice* device);

        private:
            Bitmap* cubemap = nullptr;
            uint32_t width = 0;
            uint32_t height = 0;
            ImageFormat format = ImageFormat::Unknown;

            uint32_t mipLevels = 1;

            AllocatedImage allocatedImage{};
            VkImageView imageView{};
            VkDeviceMemory imageMemory{};
            VkSampler sampler{};
            ImageResource imageResource{};

            std::vector<std::array<VkImageView, 6>> mipmapFaceImageViews;
        };

    public:
        VulkanCubeMapTexture(VulkanDevice* _device, const AllocatedImage _image, const VkImageView _imageView,
                             const std::vector<std::array<VkImageView, 6>> _mipmapFaceImageViews, const VkSampler _sampler,
                             const VkDeviceMemory _imageMemory): device(_device),
                                                                 allocatedImage(_image),
                                                                 imageView(_imageView),
                                                                 mipmapFaceImageViews(_mipmapFaceImageViews),
                                                                 imageMemory(_imageMemory),
                                                                 sampler(_sampler) {}

        ~VulkanCubeMapTexture();

        VkImage GetImage() const { return allocatedImage.image; };
        VkImageView GetImageView() const { return imageView; }
        VkImageView GetFaceImageView(size_t faceIndex, uint32_t mipLevel) const { return mipmapFaceImageViews[mipLevel][faceIndex]; }
        VkSampler GetSampler() const { return sampler; }
        uint32_t GetWidth() const { return allocatedImage.width; }
        uint32_t GetHeight() const { return allocatedImage.height; }

    private:
        VulkanDevice* device;

        AllocatedImage allocatedImage{};
        VkImageView imageView{};
        std::vector<std::array<VkImageView, 6>> mipmapFaceImageViews;
        VkDeviceMemory imageMemory{};
        VkSampler sampler{};
    };
}
