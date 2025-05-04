//
// Created by peter on 2025. 04. 27..
//

#include "vulkan_cube_map_texture.h"

#include "renderer/bitmap.h"

namespace Raytracing
{
    Ref<VulkanCubeMapTexture> VulkanCubeMapTexture::Builder::Build(VulkanDevice* device)
    {
        ASSERT(data && size > 0, "Texture data is NULL or size is 0.")

        allocatedImage = ImageBuilder(device)
                .SetFormat(format)
                .SetResolution(width, height)
                .SetTiling(VK_IMAGE_TILING_OPTIMAL)
                .AddUsage(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
                .SetArrayLayers(6)
                .SetFlags(VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT)
                .Build();

        imageView = ImageViewBuilder(device)
                .SetFormat(format)
                .SetImage(allocatedImage.image)
                .SetViewType(VK_IMAGE_VIEW_TYPE_CUBE)
                .SetAspectFlags(VK_IMAGE_ASPECT_COLOR_BIT)
                .SetLayerCount(6)
                .Build();

        for (size_t i = 0; i < 6; i++)
        {
            faceImageViews[i] = ImageViewBuilder(device)
                    .SetFormat(format)
                    .SetImage(allocatedImage.image)
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


        if (cubemap)
        {
            const auto stagingBuffer = VulkanBuffer(device, cubemap->pixelData.size(),
                                                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                                    VMA_MEMORY_USAGE_CPU_ONLY);

            memcpy(stagingBuffer.GetMappedData(), cubemap->pixelData.data(), cubemap->pixelData.size());

            std::vector<VkBufferImageCopy> bufferCopyRegions;
            for (uint32_t face = 0; face < 6; face++)
            {
                const uint64_t offset = Bitmap::GetImageOffsetForFace(*cubemap, face);
                VkBufferImageCopy bufferCopyRegion = {};
                bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                bufferCopyRegion.imageSubresource.mipLevel = 0;
                bufferCopyRegion.imageSubresource.baseArrayLayer = face;
                bufferCopyRegion.imageSubresource.layerCount = 1;
                bufferCopyRegion.imageExtent = {width, height, 1};
                bufferCopyRegion.bufferOffset = offset;
                bufferCopyRegions.push_back(bufferCopyRegion);
            }

            device->ImmediateSubmit([&](const VkCommandBuffer cmd) {
                VulkanUtils::TransitionImageLayout(cmd, allocatedImage.image, VK_IMAGE_ASPECT_COLOR_BIT,
                                                   VK_IMAGE_LAYOUT_UNDEFINED,
                                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            });

            device->ImmediateSubmit([&](const VkCommandBuffer cmd) {
                vkCmdCopyBufferToImage(
                    cmd,
                    stagingBuffer.GetBuffer(),
                    allocatedImage.image,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    6,
                    bufferCopyRegions.data()
                );
            });

            device->ImmediateSubmit([&](const VkCommandBuffer cmd) {
                VulkanUtils::TransitionImageLayout(cmd, allocatedImage.image, VK_IMAGE_ASPECT_COLOR_BIT,
                                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            });
        }

        return CreateRef<VulkanCubeMapTexture>(device, allocatedImage, imageView, faceImageViews, sampler, imageMemory);
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
