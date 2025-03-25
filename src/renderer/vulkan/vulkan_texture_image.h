#pragma once
#include <string>

#include "vulkan_image.h"

namespace Raytracing
{
    class VulkanTextureImage : public VulkanImage {
    public:
        VulkanTextureImage(VulkanDevice *device, const std::string &image_path);
        ~VulkanTextureImage() override = default;

    private:
        void CreateTextureImage(const std::string &image_path);
    };
}
