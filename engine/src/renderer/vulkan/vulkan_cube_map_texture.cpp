#include "renderer/vulkan/vulkan_cube_map_texture.h"

#include "renderer/vulkan/vulkan_buffer.h"
#include "renderer/bitmap.h"

namespace MongooseVK
{
    Ref<VulkanCubeMapTexture> VulkanCubeMapTexture::Builder::Build(VulkanDevice* device)
    {
        constexpr uint32_t MAX_MIP_LEVEL = 6;

        mipLevels = std::min(mipLevels, MAX_MIP_LEVEL);

        allocatedImage = ImageBuilder(device)
                .SetFormat(format)
                .SetResolution(width, height)
                .SetTiling(VK_IMAGE_TILING_OPTIMAL)
                .AddUsage(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
                .SetArrayLayers(6)
                .SetFlags(VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT)
                .SetMipLevels(mipLevels)
                .Build();

        imageView = ImageViewBuilder(device)
                .SetFormat(format)
                .SetImage(allocatedImage.image)
                .SetViewType(VK_IMAGE_VIEW_TYPE_CUBE)
                .SetAspectFlags(VK_IMAGE_ASPECT_COLOR_BIT)
                .SetLayerCount(6)
                .SetMipLevels(mipLevels)
                .Build();


        mipmapFaceImageViews.resize(mipLevels);
        for (size_t miplevel = 0; miplevel < mipLevels; miplevel++)
        {
            for (size_t faceindex = 0; faceindex < 6; faceindex++)
            {
                mipmapFaceImageViews[miplevel][faceindex] = ImageViewBuilder(device)
                        .SetFormat(format)
                        .SetImage(allocatedImage.image)
                        .SetViewType(VK_IMAGE_VIEW_TYPE_2D)
                        .SetAspectFlags(VK_IMAGE_ASPECT_COLOR_BIT)
                        .SetLayerCount(1)
                        .SetBaseArrayLayer(faceindex)
                        .SetBaseMipLevel(miplevel)
                        .Build();
            }
        }

        sampler = ImageSamplerBuilder(device)
                .SetFormat(format)
                .SetMipLevels(mipLevels)
                .SetFilter(VK_FILTER_LINEAR, VK_FILTER_LINEAR)
                .SetAddressMode(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
                .SetBorderColor(VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK)
                .Build();


        if (cubemap)
        {
            auto stagingBuffer = device->CreateBuffer(cubemap->pixelData.size(),
                                                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                                        VMA_MEMORY_USAGE_CPU_ONLY);

            memcpy(stagingBuffer.GetData(), cubemap->pixelData.data(), cubemap->pixelData.size());

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
                    stagingBuffer.buffer,
                    allocatedImage.image,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    6,
                    bufferCopyRegions.data()
                );
            });

            device->ImmediateSubmit([&](const VkCommandBuffer cmd) {
                VulkanUtils::GenerateCubemapMipmaps(cmd,
                                                    device->GetPhysicalDevice(),
                                                    allocatedImage.image,
                                                    VulkanUtils::ConvertImageFormat(format),
                                                    width,
                                                    height,
                                                    mipLevels);
            });

            device->DestroyBuffer(stagingBuffer);
        }

        return CreateRef<VulkanCubeMapTexture>(device, allocatedImage, imageView, mipmapFaceImageViews, sampler, imageMemory);
    }

    VulkanCubeMapTexture::~VulkanCubeMapTexture()
    {
        vkDestroySampler(device->GetDevice(), sampler, nullptr);
        vkDestroyImageView(device->GetDevice(), imageView, nullptr);

        for (size_t j = 0; j < mipmapFaceImageViews.size(); j++)
            for (size_t i = 0; i < 6; i++)
                vkDestroyImageView(device->GetDevice(), mipmapFaceImageViews[j][i], nullptr);

        vmaDestroyImage(device->GetVmaAllocator(), allocatedImage.image, allocatedImage.allocation);
    }
}
