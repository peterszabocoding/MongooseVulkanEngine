#include "renderer/vulkan/vulkan_image.h"
#include "renderer/vulkan/vulkan_utils.h"
#include "renderer/vulkan/vulkan_device.h"
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
                case VK_FORMAT_R32G32B32_UINT:
                case VK_FORMAT_R32G32B32_SINT:
                    return VK_BORDER_COLOR_INT_OPAQUE_BLACK;

                case VK_FORMAT_R32G32B32A32_SFLOAT:
                case VK_FORMAT_R32G32B32_SFLOAT:
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
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = arrayLayers;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = samples;
        imageInfo.sharingMode = sharingMode;

        if (flags)
            imageInfo.flags = flags;


        AllocatedImage allocatedImage;
        allocatedImage.width = width;
        allocatedImage.height = height;

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
        viewInfo.subresourceRange.baseMipLevel = baseMipLevel;
        viewInfo.subresourceRange.levelCount = mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = baseArrayLayer;
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
        samplerInfo.borderColor = borderColor;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = compareEnabled ? VK_TRUE : VK_FALSE;
        samplerInfo.compareOp = compareOp;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = static_cast<float>(mipLevels);

        VkSampler sampler;
        VK_CHECK_MSG(vkCreateSampler(device->GetDevice(), &samplerInfo, nullptr, &sampler), "Failed to create texture sampler.");

        return sampler;
    }
}
