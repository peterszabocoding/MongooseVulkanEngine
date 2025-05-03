//
// Created by peter on 2025. 04. 27..
//

#include "vulkan_cube_map_texture.h"

namespace Raytracing
{
    Ref<VulkanCubeMapTexture> VulkanCubeMapTexture::Builder::Build(VulkanDevice* device)
    {
        ASSERT(data && size > 0, "Texture data is NULL or size is 0.")

        image = ImageBuilder(device)
                .SetFormat(format)
                .SetResolution(width, height)
                .SetTiling(VK_IMAGE_TILING_OPTIMAL)
                .AddUsage(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
                .SetArrayLayers(6)
                .SetFlags(VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT)
                .Build();

        imageView = ImageViewBuilder(device)
                .SetFormat(format)
                .SetImage(image.image)
                .SetViewType(VK_IMAGE_VIEW_TYPE_CUBE)
                .SetAspectFlags(VK_IMAGE_ASPECT_COLOR_BIT)
                .SetLayerCount(6)
                .Build();

        for (size_t i = 0; i < 6; i++)
        {
            faceImageViews[i] = ImageViewBuilder(device)
                    .SetFormat(format)
                    .SetImage(image.image)
                    .SetViewType(VK_IMAGE_VIEW_TYPE_2D)
                    .SetAspectFlags(VK_IMAGE_ASPECT_COLOR_BIT)
                    .SetLayerCount(1)
                    .SetBaseArrayLayer(i)
                    .Build();
        }

        sampler = ImageSamplerBuilder(device)
                .SetFilter(VK_FILTER_LINEAR, VK_FILTER_LINEAR)
                .SetAddressMode(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
                .SetFormat(format)
                .SetBorderColor(VK_BORDER_COLOR_INT_OPAQUE_WHITE)
                .Build();

        return CreateRef<VulkanCubeMapTexture>(device, image, imageView, faceImageViews, sampler, imageMemory);
    }

    VulkanCubeMapTexture::~VulkanCubeMapTexture()
    {
        vkDestroySampler(device->GetDevice(), sampler, nullptr);
        vkDestroyImageView(device->GetDevice(), imageView, nullptr);

        for (size_t i = 0; i < 6; i++)
        {
            vkDestroyImageView(device->GetDevice(), faceImageViews[i], nullptr);
        }

        vmaDestroyImage(device->GetVmaAllocator(), allocatedImage.image, allocatedImage.allocation);
    }
}
