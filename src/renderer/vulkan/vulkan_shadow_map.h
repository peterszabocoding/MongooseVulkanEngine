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
            VkImageView imageView{};
            std::vector<VkImageView> arrayImageViews{};
            VkDeviceMemory imageMemory{};
            VkSampler sampler{};
        };

    public:
        VulkanShadowMap(VulkanDevice* _device, const AllocatedImage& _image, const VkImageLayout _imageLayout, const VkImageView _imageView,
                        const std::vector<VkImageView> _arrayImageViews,
                        const VkSampler _sampler): device(_device),
                                                   allocatedImage(_image),
                                                   imageView(_imageView),
                                                   arrayImageViews(_arrayImageViews),
                                                   sampler(_sampler),
                                                   imageLayout(_imageLayout) {}

        ~VulkanShadowMap();

        VkImage GetImage() const { return allocatedImage.image; };
        VkImageView GetImageView() const { return imageView; }
        VkImageView GetImageView(uint32_t index) const { return arrayImageViews[index % arrayImageViews.size()]; }
        VkSampler GetSampler() const { return sampler; }
        VkImageLayout GetImageLayout() const { return imageLayout; }

        void TransitionToDepthRendering(VkCommandBuffer cmd);
        void TransitionToShaderRead(VkCommandBuffer cmd);

    private:
        VulkanDevice* device;

        AllocatedImage allocatedImage{};
        VkImageView imageView{};
        std::vector<VkImageView> arrayImageViews{};
        VkSampler sampler{};
        VkImageLayout imageLayout{};
    };
}
