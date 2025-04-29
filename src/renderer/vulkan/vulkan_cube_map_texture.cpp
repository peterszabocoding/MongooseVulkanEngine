//
// Created by peter on 2025. 04. 27..
//

#include "vulkan_cube_map_texture.h"

namespace Raytracing
{
    Ref<VulkanCubeMapTexture> VulkanCubeMapTexture::Builder::Build(VulkanDevice* device)
    {
        ASSERT(data && size > 0, "Texture data is NULL or size is 0.")

        AllocatedImage allocatedImage = ImageBuilder(device)
                .SetFormat(format)
                .SetResolution(width, height)
                .SetTiling(VK_IMAGE_TILING_OPTIMAL)
                .AddUsage(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
                .SetArrayLayers(6)
                .SetFlags(VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT)
                .Build();

        image = allocatedImage.image;
        imageMemory = allocatedImage.imageMemory;

        imageView = ImageViewBuilder(device)
                .SetFormat(format)
                .SetImage(image)
                .SetViewType(VK_IMAGE_VIEW_TYPE_CUBE)
                .SetAspectFlags(VK_IMAGE_ASPECT_COLOR_BIT)
                .SetLayerCount(6)
                .Build();

        sampler = ImageSamplerBuilder(device)
                .SetFilter(VK_FILTER_LINEAR, VK_FILTER_LINEAR)
                .SetAddressMode(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
                .Build();

        return CreateRef<VulkanCubeMapTexture>(device, image, imageView, sampler, imageMemory);
    }

    VulkanCubeMapTexture::~VulkanCubeMapTexture()
    {
        vkDestroySampler(device->GetDevice(), sampler, nullptr);
        vkFreeMemory(device->GetDevice(), imageMemory, nullptr);
        vkDestroyImageView(device->GetDevice(), imageView, nullptr);
        vkDestroyImage(device->GetDevice(), image, nullptr);
    }
}
