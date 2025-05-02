#include "vulkan_image.h"
#include "vulkan_utils.h"
#include "vulkan_device.h"
#include "util/log.h"

namespace Raytracing
{
    namespace ImageUtils
    {
        static VkBorderColor GetBorderColor(const VkFormat format)
        {
            switch (format)
            {
                case VK_FORMAT_R8G8B8A8_SRGB:
                case VK_FORMAT_R8G8B8A8_UNORM:
                    return VK_BORDER_COLOR_INT_OPAQUE_BLACK;

                case VK_FORMAT_R32G32B32A32_SFLOAT:
                    return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;

                default:
                    return VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            }
        }
    }

    AllocatedImage ImageBuilder::Build()
    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = arrayLayers;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (flags)
            imageInfo.flags = flags;


        AllocatedImage allocatedImage;

        VmaAllocationCreateInfo vmaallocInfo = {};
        vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        VK_CHECK_MSG(vmaCreateImage(device->GetVmaAllocator(), &imageInfo, &vmaallocInfo,
                         &allocatedImage.image, &allocatedImage.allocation, &allocatedImage.allocationInfo),
                     "Failed to create image.");

        return allocatedImage;
    }

    VkImageView ImageViewBuilder::Build() const
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = viewType;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = layerCount;

        VkImageView imageView = VK_NULL_HANDLE;
        VK_CHECK_MSG(vkCreateImageView(device->GetDevice(), &viewInfo, nullptr, &imageView), "Failed to create texture image view.");

        return imageView;
    }

    VkSampler ImageSamplerBuilder::Build()
    {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(device->GetPhysicalDevice(), &properties);

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = magFilter;
        samplerInfo.minFilter = minFilter;

        samplerInfo.addressModeU = addressMode;
        samplerInfo.addressModeV = addressMode;
        samplerInfo.addressModeW = addressMode;

        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = ImageUtils::GetBorderColor(format);
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

        VkSampler sampler;
        VK_CHECK_MSG(vkCreateSampler(device->GetDevice(), &samplerInfo, nullptr, &sampler), "Failed to create texture sampler.");

        return sampler;
    }
}
