#pragma once

#include "vulkan_image.h"

namespace Raytracing
{
    class VulkanShadowMap {
    public:
        class Builder {
        public:
            Builder() = default;
            ~Builder() = default;

            Builder& SetResolution(const uint32_t _width, const uint32_t _height)
            {
                width = _width;
                height = _height;
                return *this;
            }

            Ref<VulkanShadowMap> Build(VulkanDevice* device);

        private:
            uint32_t width, height;
            AllocatedImage allocatedImage{};
            VkImageView imageView{};
            VkDeviceMemory imageMemory{};
            VkSampler sampler{};
        };

    public:
        VulkanShadowMap(VulkanDevice* _device, const AllocatedImage& _image, const VkImageLayout _imageLayout, const VkImageView _imageView,
                        const VkSampler _sampler): device(_device),
                                                   allocatedImage(_image),
                                                   imageLayout(_imageLayout),
                                                   imageView(_imageView),
                                                   sampler(_sampler) {}

        ~VulkanShadowMap();

        VkImage GetImage() const { return allocatedImage.image; };
        VkImageView GetImageView() const { return imageView; }
        VkSampler GetSampler() const { return sampler; }
        VkImageLayout GetImageLayout() const { return imageLayout; }

        void PrepareToDepthRendering();
        void PrepareToShadowRendering();

    private:
        VulkanDevice* device;

        AllocatedImage allocatedImage{};
        VkImageView imageView{};
        VkSampler sampler{};
        VkImageLayout imageLayout{};
    };
}
