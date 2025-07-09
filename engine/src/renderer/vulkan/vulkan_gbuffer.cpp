#include "renderer/vulkan/vulkan_gbuffer.h"

#include "renderer/vulkan/vulkan_descriptor_set_layout.h"
#include "renderer/vulkan/vulkan_image.h"

namespace MongooseVK
{
    VulkanGBuffer::Builder& VulkanGBuffer::Builder::SetResolution(const uint32_t _width, const uint32_t _height)
    {
        width = _width;
        height = _height;

        return *this;
    }

    Ref<VulkanGBuffer> VulkanGBuffer::Builder::Build(VulkanDevice* device)
    {
        Buffers buffers;
        buffers.viewSpaceNormal = CreateAttachment(device, ImageFormat::RGBA32_SFLOAT);
        buffers.viewSpacePosition = CreateAttachment(device, ImageFormat::RGBA32_SFLOAT);
        buffers.depth = CreateAttachment(device, ImageFormat::DEPTH32);

        return CreateRef<VulkanGBuffer>(device, width, height, buffers);
    }

    BufferAttachment VulkanGBuffer::Builder::CreateAttachment(VulkanDevice* device, ImageFormat format)
    {
        const VkImageLayout imageLayout = ImageUtils::GetLayoutFromFormat(format);
        const AllocatedImage allocatedImage = ImageBuilder(device)
                .SetResolution(width, height)
                .SetTiling(VK_IMAGE_TILING_OPTIMAL)
                .SetFormat(format)
                .AddUsage(ImageUtils::GetUsageFromFormat(format))
                .AddUsage(VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
                .SetInitialLayout(imageLayout)
                .Build();

        const VkImageView imageView = ImageViewBuilder(device)
                .SetFormat(format)
                .SetAspectFlags(ImageUtils::GetAspectFlagFromFormat(format))
                .SetImage(allocatedImage.image)
                .Build();

        const VkSampler sampler = ImageSamplerBuilder(device)
                .SetFilter(VK_FILTER_LINEAR, VK_FILTER_LINEAR)
                .SetFormat(format)
                .SetAddressMode(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
                .SetBorderColor(VK_BORDER_COLOR_INT_OPAQUE_BLACK)
                .Build();

        return {
            allocatedImage,
            imageView,
            sampler,
            imageLayout,
            VulkanUtils::ConvertImageFormat(format)
        };
    }

    VulkanGBuffer::~VulkanGBuffer()
    {
        vkDestroyImageView(vulkanDevice->GetDevice(), buffers.viewSpaceNormal.imageView, nullptr);
        vmaDestroyImage(vulkanDevice->GetVmaAllocator(), buffers.viewSpaceNormal.allocatedImage.image,
                        buffers.viewSpaceNormal.allocatedImage.allocation);

        vkDestroyImageView(vulkanDevice->GetDevice(), buffers.viewSpacePosition.imageView, nullptr);
        vmaDestroyImage(vulkanDevice->GetVmaAllocator(), buffers.viewSpacePosition.allocatedImage.image,
                        buffers.viewSpacePosition.allocatedImage.allocation);

        vkDestroyImageView(vulkanDevice->GetDevice(), buffers.depth.imageView, nullptr);
        vmaDestroyImage(vulkanDevice->GetVmaAllocator(), buffers.depth.allocatedImage.image,
                        buffers.depth.allocatedImage.allocation);
    }
}
