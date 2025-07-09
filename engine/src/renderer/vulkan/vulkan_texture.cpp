#include "renderer/vulkan/vulkan_texture.h"
#include "renderer/vulkan/vulkan_buffer.h"
#include "renderer/vulkan/vulkan_device.h"
#include "renderer/vulkan/vulkan_image.h"
#include "renderer/vulkan/vulkan_utils.h"

namespace MongooseVK
{
    Ref<VulkanTexture> VulkanTextureBuilder::Build(VulkanDevice* device)
    {
        allocatedImage = ImageBuilder(device)
                .SetFormat(format)
                .SetResolution(width, height)
                .SetTiling(tiling)
                .AddUsage(usage)
                .AddUsage(VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
                .SetInitialLayout(VK_IMAGE_LAYOUT_UNDEFINED)
                .SetMipLevels(mipLevels)
                .SetArrayLayers(arrayLayers)
                .Build();

        for (size_t i = 0; i < arrayLayers; i++)
        {
            imageViews[i] = ImageViewBuilder(device)
                    .SetFormat(format)
                    .SetImage(allocatedImage.image)
                    .SetViewType(VK_IMAGE_VIEW_TYPE_2D)
                    .SetAspectFlags(aspectFlags)
                    .SetMipLevels(mipLevels)
                    .SetBaseArrayLayer(i)
                    .Build();
        }

        sampler = ImageSamplerBuilder(device)
                .SetFilter(minFilter, magFilter)
                .SetFormat(format)
                .SetMipLevels(mipLevels)
                .Build();

        if (data && size > 0)
        {
            auto stagingBuffer = device->AllocateBuffer(size,
                                                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                                        VMA_MEMORY_USAGE_CPU_ONLY);

            memcpy(stagingBuffer.GetData(), data, stagingBuffer.GetBufferSize());

            device->ImmediateSubmit([&](const VkCommandBuffer cmd) {
                VulkanUtils::TransitionImageLayout(cmd, allocatedImage.image,
                                                   VK_IMAGE_ASPECT_COLOR_BIT,
                                                   VK_IMAGE_LAYOUT_UNDEFINED,
                                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                   mipLevels);
            });

            device->ImmediateSubmit([&](const VkCommandBuffer cmd) {
                VulkanUtils::CopyBufferToImage(cmd, allocatedImage.image, width, height, stagingBuffer.buffer);
            });


            if (mipLevels > 1)
            {
                device->ImmediateSubmit([&](const VkCommandBuffer cmd) {
                    VulkanUtils::GenerateMipmaps(cmd,
                                                 device->GetPhysicalDevice(),
                                                 allocatedImage.image,
                                                 VulkanUtils::ConvertImageFormat(format),
                                                 width,
                                                 height,
                                                 mipLevels);
                });
            } else
            {
                device->ImmediateSubmit([&](const VkCommandBuffer cmd) {
                    VulkanUtils::TransitionImageLayout(cmd, allocatedImage.image,
                                                       VK_IMAGE_ASPECT_COLOR_BIT,
                                                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                       mipLevels);
                });
            }

            device->FreeBuffer(stagingBuffer);
        }

        imageResource.data = data;
        imageResource.size = size;
        imageResource.format = format;
        imageResource.width = width;
        imageResource.height = height;


        return CreateRef<VulkanTexture>(allocatedImage, imageViews, sampler, imageMemory, imageResource);
    }

    void VulkanTextureBuilder::Build(VulkanDevice* device, VulkanTexture& texture)
    {
        allocatedImage = ImageBuilder(device)
                .SetFormat(format)
                .SetResolution(width, height)
                .SetTiling(tiling)
                .AddUsage(usage)
                .AddUsage(VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
                .SetInitialLayout(VK_IMAGE_LAYOUT_UNDEFINED)
                .SetMipLevels(mipLevels)
                .SetArrayLayers(arrayLayers)
                .Build();

        for (size_t i = 0; i < arrayLayers; i++)
        {
            imageViews[i] = ImageViewBuilder(device)
                    .SetFormat(format)
                    .SetImage(allocatedImage.image)
                    .SetViewType(VK_IMAGE_VIEW_TYPE_2D)
                    .SetAspectFlags(aspectFlags)
                    .SetMipLevels(mipLevels)
                    .SetBaseArrayLayer(i)
                    .Build();
        }

        sampler = ImageSamplerBuilder(device)
                .SetFilter(minFilter, magFilter)
                .SetFormat(format)
                .SetMipLevels(mipLevels)
                .Build();

        if (data && size > 0)
        {
            auto stagingBuffer = device->AllocateBuffer(size,
                                                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                                        VMA_MEMORY_USAGE_CPU_ONLY);

            memcpy(stagingBuffer.GetData(), data, stagingBuffer.GetBufferSize());

            device->ImmediateSubmit([&](const VkCommandBuffer cmd) {
                VulkanUtils::TransitionImageLayout(cmd, allocatedImage.image,
                                                   VK_IMAGE_ASPECT_COLOR_BIT,
                                                   VK_IMAGE_LAYOUT_UNDEFINED,
                                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                   mipLevels);
            });

            device->ImmediateSubmit([&](const VkCommandBuffer cmd) {
                VulkanUtils::CopyBufferToImage(cmd, allocatedImage.image, width, height, stagingBuffer.buffer);
            });

            device->ImmediateSubmit([&](const VkCommandBuffer cmd) {
                if (mipLevels > 1)
                {
                    VulkanUtils::GenerateMipmaps(cmd,
                                                 device->GetPhysicalDevice(),
                                                 allocatedImage.image,
                                                 VulkanUtils::ConvertImageFormat(format),
                                                 width,
                                                 height,
                                                 mipLevels);
                } else
                {
                    VulkanUtils::TransitionImageLayout(cmd, allocatedImage.image,
                                                       VK_IMAGE_ASPECT_COLOR_BIT,
                                                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                       mipLevels);
                }
            });

            device->FreeBuffer(stagingBuffer);
        }

        imageResource.data = data;
        imageResource.size = size;
        imageResource.format = format;
        imageResource.width = width;
        imageResource.height = height;

        texture.sampler = sampler;
        texture.imageViews = imageViews;
        texture.imageMemory = imageMemory;
        texture.imageResource = imageResource;
        texture.allocatedImage = allocatedImage;
    }
}
