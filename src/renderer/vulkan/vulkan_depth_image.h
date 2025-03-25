#pragma once

#include "vulkan_image.h"

namespace Raytracing
{
    class VulkanDepthImage: public VulkanImage {
    public:
        VulkanDepthImage(VulkanDevice* device, uint32_t width, uint32_t height);
        ~VulkanDepthImage() override = default;

        void Resize(uint32_t width, uint32_t height);

    private:
        void CreateDepthImage(uint32_t width, uint32_t height);

    };
}
