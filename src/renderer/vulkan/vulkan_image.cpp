#include "vulkan_image.h"

#include "vulkan_buffer.h"
#include "vulkan_utils.h"
#include "vulkan_device.h"
#include "vulkan_image_view.h"
#include "util/log.h"

namespace Raytracing
{
    Ref<VulkanImage> VulkanImage::Builder::Build()
    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VK_CHECK_MSG(vkCreateImage(device->GetDevice(), &imageInfo, nullptr, &image), "Failed to create image.");

        imageMemory = VulkanUtils::AllocateImageMemory(device, image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        imageView = VulkanImageView::Builder(device)
                .SetFormat(format)
                .SetImage(image)
                .SetViewType(VK_IMAGE_VIEW_TYPE_2D)
                .SetAspectFlags(aspectFlags)
                .Build();

        if (makeSampler)
        {
            VkPhysicalDeviceProperties properties{};
            vkGetPhysicalDeviceProperties(device->GetPhysicalDevice(), &properties);

            VkSamplerCreateInfo samplerInfo{};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.magFilter = magFilter;
            samplerInfo.minFilter = minFilter;

            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

            samplerInfo.anisotropyEnable = VK_TRUE;
            samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
            samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            samplerInfo.unnormalizedCoordinates = VK_FALSE;
            samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerInfo.mipLodBias = 0.0f;
            samplerInfo.minLod = 0.0f;
            samplerInfo.maxLod = 0.0f;

            VK_CHECK_MSG(vkCreateSampler(device->GetDevice(), &samplerInfo, nullptr, &sampler), "Failed to create texture sampler.");
        }

        if (data && size > 0)
        {
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
        }
        return CreateRef<VulkanImage>(device, image, imageView, imageMemory, sampler);
    }

    VulkanImage::~VulkanImage()
    {
        vkDestroySampler(device->GetDevice(), sampler, nullptr);
        vkFreeMemory(device->GetDevice(), imageMemory, nullptr);
        vkDestroyImage(device->GetDevice(), image, nullptr);
    }
}
