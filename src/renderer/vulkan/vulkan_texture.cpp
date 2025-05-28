#include "vulkan_texture.h"

#include "vulkan_buffer.h"
#include "vulkan_device.h"
#include "vulkan_image.h"
#include "vulkan_utils.h"

namespace Raytracing
{
    Ref<VulkanTexture> VulkanTexture::Builder::Build(VulkanDevice* device)
    {
        allocatedImage = ImageBuilder(device)
                .SetFormat(format)
                .SetResolution(width, height)
                .SetTiling(tiling)
                .AddUsage(usage)
                .AddUsage(VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
                .SetInitialLayout(VK_IMAGE_LAYOUT_UNDEFINED)
                .SetMipLevels(mipLevels)
                .Build();

        imageView = ImageViewBuilder(device)
                .SetFormat(format)
                .SetImage(allocatedImage.image)
                .SetViewType(VK_IMAGE_VIEW_TYPE_2D)
                .SetAspectFlags(aspectFlags)
                .SetMipLevels(mipLevels)
                .Build();

        sampler = ImageSamplerBuilder(device)
                .SetFilter(minFilter, magFilter)
                .SetFormat(format)
                .SetMipLevels(mipLevels)
                .Build();

        if (data && size > 0)
        {
            auto stagingBuffer = VulkanBuffer(device, size,
                                              VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                              VMA_MEMORY_USAGE_CPU_ONLY);

            memcpy(stagingBuffer.GetMappedData(), data, stagingBuffer.GetBufferSize());

            device->ImmediateSubmit([&](const VkCommandBuffer cmd) {
                VulkanUtils::TransitionImageLayout(cmd, allocatedImage.image,
                                                   VK_IMAGE_ASPECT_COLOR_BIT,
                                                   VK_IMAGE_LAYOUT_UNDEFINED,
                                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                   mipLevels);
            });

            device->ImmediateSubmit([&](const VkCommandBuffer cmd) {
                VulkanUtils::CopyBufferToImage(cmd, allocatedImage.image, width, height, stagingBuffer.GetBuffer());
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
        }

        return CreateRef<VulkanTexture>(device, allocatedImage, imageView, sampler, imageMemory, imageResource);
    }

    VulkanTexture::~VulkanTexture()
    {
        vkDestroySampler(device->GetDevice(), sampler, nullptr);
        vkDestroyImageView(device->GetDevice(), imageView, nullptr);
        vmaDestroyImage(device->GetVmaAllocator(), allocatedImage.image, allocatedImage.allocation);
    }
}
