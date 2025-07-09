#include "renderer/vulkan/vulkan_shadow_map.h"

#include "renderer/vulkan/vulkan_device.h"

namespace MongooseVK
{
    Ref<VulkanShadowMap> VulkanShadowMap::Builder::Build(VulkanDevice* device)
    {
        allocatedImage = ImageBuilder(device)
                .SetFormat(ImageFormat::DEPTH32)
                .SetResolution(width, height)
                .SetTiling(VK_IMAGE_TILING_OPTIMAL)
                .AddUsage(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT)
                .SetInitialLayout(VK_IMAGE_LAYOUT_UNDEFINED)
                .SetArrayLayers(arrayLayers)
                .Build();

        imageView = ImageViewBuilder(device)
                .SetFormat(ImageFormat::DEPTH32)
                .SetImage(allocatedImage.image)
                .SetViewType(arrayLayers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D)
                .SetAspectFlags(VK_IMAGE_ASPECT_DEPTH_BIT)
                .SetBaseArrayLayer(0)
                .SetLayerCount(arrayLayers)
                .Build();

        arrayImageViews.resize(arrayLayers);
        for (uint32_t i = 0; i < arrayLayers; i++)
        {
            arrayImageViews[i] = ImageViewBuilder(device)
                    .SetFormat(ImageFormat::DEPTH32)
                    .SetImage(allocatedImage.image)
                    .SetViewType(VK_IMAGE_VIEW_TYPE_2D)
                    .SetAspectFlags(VK_IMAGE_ASPECT_DEPTH_BIT)
                    .SetBaseArrayLayer(i)
                    .SetLayerCount(1)
                    .Build();
        }

        sampler = ImageSamplerBuilder(device)
                .SetFilter(VK_FILTER_LINEAR, VK_FILTER_LINEAR)
                .SetFormat(ImageFormat::DEPTH32)
                .SetCompareOp(true, VK_COMPARE_OP_LESS)
                .SetAddressMode(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
                .SetBorderColor(VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE)
                .Build();

        return CreateRef<VulkanShadowMap>(device, allocatedImage, VK_IMAGE_LAYOUT_UNDEFINED, imageView, arrayImageViews, sampler);
    }

    VulkanShadowMap::~VulkanShadowMap()
    {
        vkDestroySampler(device->GetDevice(), sampler, nullptr);
        for (const auto imageView: arrayImageViews)
        {
            vkDestroyImageView(device->GetDevice(), imageView, nullptr);
        }
        vmaDestroyImage(device->GetVmaAllocator(), allocatedImage.image, allocatedImage.allocation);
    }

    void VulkanShadowMap::TransitionToDepthRendering(VkCommandBuffer cmd)
    {
        /*
        VulkanUtils::TransitionImageLayout(cmd, allocatedImage.image, VK_IMAGE_ASPECT_DEPTH_BIT, imageLayout,
                                           VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
*/


        VkImageSubresourceRange subresourceRange;
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = 1;
        subresourceRange.baseArrayLayer = 0;
        subresourceRange.layerCount = arrayImageViews.size();

        VulkanUtils::InsertImageMemoryBarrier(cmd,
            allocatedImage.image,
            VK_ACCESS_SHADER_READ_BIT,
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            imageLayout,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, subresourceRange);


        imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    void VulkanShadowMap::TransitionToShaderRead(VkCommandBuffer cmd)
    {
        /*
        VulkanUtils::TransitionImageLayout(cmd, allocatedImage.image, VK_IMAGE_ASPECT_DEPTH_BIT, imageLayout,
                                           VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                                           */

        VkImageSubresourceRange subresourceRange;
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = 1;
        subresourceRange.baseArrayLayer = 0;
        subresourceRange.layerCount = arrayImageViews.size();

        VulkanUtils::InsertImageMemoryBarrier(cmd,
            allocatedImage.image,
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_ACCESS_SHADER_READ_BIT,
            imageLayout,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, subresourceRange);

        imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
}
