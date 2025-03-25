#include "vulkan_depth_image.h"

#include "vulkan_utils.h"
#include "vulkan_device.h"

namespace Raytracing
{
    VulkanDepthImage::VulkanDepthImage(VulkanDevice *device, uint32_t width, uint32_t height): VulkanImage(device)
    {
        CreateDepthImage(width, height);
        CreateTextureImageView(VK_FORMAT_D24_UNORM_S8_UINT, VK_IMAGE_ASPECT_DEPTH_BIT);
        CreateTextureSampler();
    }

    void VulkanDepthImage::Resize(uint32_t width, uint32_t height)
    {
        CreateDepthImage(width, height);
        CreateTextureImageView(VK_FORMAT_D24_UNORM_S8_UINT, VK_IMAGE_ASPECT_DEPTH_BIT);
        CreateTextureSampler();
    }

    void VulkanDepthImage::CreateDepthImage(uint32_t width, uint32_t height)
    {
        CreateImage(width, height,
                    VK_FORMAT_D24_UNORM_S8_UINT,
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    textureImage,
                    textureImageMemory);
    }
}
