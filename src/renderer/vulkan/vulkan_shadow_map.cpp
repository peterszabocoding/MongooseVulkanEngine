#include "vulkan_shadow_map.h"

#include "vulkan_device.h"

namespace Raytracing
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

        imageViews.resize(arrayLayers);
        for (uint32_t i = 0; i < arrayLayers; i++)
        {
            imageViews[i] = ImageViewBuilder(device)
                    .SetFormat(ImageFormat::DEPTH32)
                    .SetImage(allocatedImage.image)
                    .SetViewType(VK_IMAGE_VIEW_TYPE_2D)
                    .SetAspectFlags(VK_IMAGE_ASPECT_DEPTH_BIT)
                    .SetBaseMipLevel(i)
                    .Build();
        }

        sampler = ImageSamplerBuilder(device)
                .SetFilter(VK_FILTER_LINEAR, VK_FILTER_LINEAR)
                .SetFormat(ImageFormat::DEPTH32)
                .SetCompareOp(true, VK_COMPARE_OP_LESS)
                .SetAddressMode(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER)
                .SetBorderColor(VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE)
                .Build();

        return CreateRef<VulkanShadowMap>(device, allocatedImage, VK_IMAGE_LAYOUT_UNDEFINED, imageViews, sampler);
    }

    VulkanShadowMap::~VulkanShadowMap()
    {
        vkDestroySampler(device->GetDevice(), sampler, nullptr);
        for (const auto imageView: imageViews)
        {
            vkDestroyImageView(device->GetDevice(), imageView, nullptr);
        }
        vmaDestroyImage(device->GetVmaAllocator(), allocatedImage.image, allocatedImage.allocation);
    }

    void VulkanShadowMap::PrepareToDepthRendering()
    {
        device->ImmediateSubmit([&](const VkCommandBuffer cmd) {
            VulkanUtils::TransitionImageLayout(cmd, allocatedImage.image, VK_IMAGE_ASPECT_DEPTH_BIT,
                                               imageLayout,
                                               VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        });
        imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    void VulkanShadowMap::PrepareToShadowRendering()
    {
        device->ImmediateSubmit([&](const VkCommandBuffer cmd) {
            VulkanUtils::TransitionImageLayout(cmd, allocatedImage.image, VK_IMAGE_ASPECT_DEPTH_BIT,
                                               imageLayout,
                                               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        });
        imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
}
