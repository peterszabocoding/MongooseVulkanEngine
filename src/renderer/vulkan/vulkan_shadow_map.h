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

            Builder& SetArrayLayers(const uint32_t _arrayLayers)
            {
                arrayLayers = _arrayLayers;
                return *this;
            }

            Ref<VulkanShadowMap> Build(VulkanDevice* device);

        private:
            uint32_t width, height;
            uint32_t arrayLayers = 1;
            AllocatedImage allocatedImage{};
            std::vector<VkImageView> imageViews{};
            VkDeviceMemory imageMemory{};
            VkSampler sampler{};
        };

    public:
        VulkanShadowMap(VulkanDevice* _device, const AllocatedImage& _image, const VkImageLayout _imageLayout, const std::vector<VkImageView> _imageViews,
                        const VkSampler _sampler): device(_device),
                                                   allocatedImage(_image),
                                                   imageViews(_imageViews),
                                                   sampler(_sampler),
                                                   imageLayout(_imageLayout) {}

        ~VulkanShadowMap();

        VkImage GetImage() const { return allocatedImage.image; };
        VkImageView GetImageView() const { return imageViews[0]; }
        VkImageView GetImageView(uint32_t index) const { return imageViews[index % imageViews.size()]; }
        VkSampler GetSampler() const { return sampler; }
        VkImageLayout GetImageLayout() const { return imageLayout; }

        void PrepareToDepthRendering();
        void PrepareToShadowRendering();

    private:
        VulkanDevice* device;

        AllocatedImage allocatedImage{};
        std::vector<VkImageView> imageViews{};
        VkSampler sampler{};
        VkImageLayout imageLayout{};
    };
}
