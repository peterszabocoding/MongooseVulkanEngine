#include "vulkan_texture.h"

#include <stdexcept>
#include "vulkan_buffer.h"
#include "vulkan_device.h"
#include "vulkan_image.h"
#include "vulkan_utils.h"

namespace Raytracing
{
    Ref<VulkanTexture> VulkanTexture::Builder::Build(VulkanDevice* device)
    {
        ASSERT(data && size > 0, "Texture data is NULL or size is 0.")

        AllocatedImage allocatedImage = ImageBuilder(device)
                .SetFormat(format)
                .SetResolution(width, height)
                .SetTiling(tiling)
                .AddUsage(usage)
                .Build();

        image = allocatedImage.image;
        imageMemory = allocatedImage.imageMemory;

        imageView = ImageViewBuilder(device)
                .SetFormat(format)
                .SetImage(image)
                .SetViewType(VK_IMAGE_VIEW_TYPE_2D)
                .SetAspectFlags(aspectFlags)
                .Build();

        sampler = ImageSamplerBuilder(device)
                .SetFilter(minFilter, magFilter)
                .SetFormat(format)
                .Build();

        const auto stagingBuffer = VulkanBuffer(device, size,
                                                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                                VMA_MEMORY_USAGE_CPU_ONLY);

        memcpy(stagingBuffer.GetMappedData(), data, stagingBuffer.GetBufferSize());

        device->ImmediateSubmit([&](const VkCommandBuffer cmd) {
            VulkanUtils::TransitionImageLayout(cmd, image, VK_IMAGE_ASPECT_COLOR_BIT,
                                               VK_IMAGE_LAYOUT_UNDEFINED,
                                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        });

        device->ImmediateSubmit([&](const VkCommandBuffer cmd) {
            VulkanUtils::CopyBufferToImage(cmd, image, width, height, stagingBuffer.GetBuffer());
        });

        device->ImmediateSubmit([&](const VkCommandBuffer cmd) {
            VulkanUtils::TransitionImageLayout(cmd, image, VK_IMAGE_ASPECT_COLOR_BIT,
                                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        });

        return CreateRef<VulkanTexture>(device, image, imageView, sampler, imageMemory, imageResource);
    }

    VulkanTexture::~VulkanTexture()
    {
        vkDestroySampler(device->GetDevice(), sampler, nullptr);
        vkFreeMemory(device->GetDevice(), imageMemory, nullptr);
        vkDestroyImageView(device->GetDevice(), imageView, nullptr);
        vkDestroyImage(device->GetDevice(), image, nullptr);
    }
}
