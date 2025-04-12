#include "vulkan_image.h"

#include "vulkan_buffer.h"
#include "vulkan_utils.h"
#include "vulkan_device.h"

namespace Raytracing
{
    VulkanImage::~VulkanImage()
    {
        vkDestroySampler(device->GetDevice(), sampler, nullptr);
        vkDestroyImageView(device->GetDevice(), imageView, nullptr);
        vkDestroyImage(device->GetDevice(), image, nullptr);
        vkFreeMemory(device->GetDevice(), imageMemory, nullptr);
    }

    VkDeviceMemory VulkanImageBuilder::AllocateImageMemory(const VulkanDevice* device, const VkImage image, const VkMemoryPropertyFlags properties)
    {
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device->GetDevice(), image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = VulkanUtils::FindMemoryType(device->GetPhysicalDevice(), memRequirements.memoryTypeBits, properties);

        VkDeviceMemory imageMemory;

        VK_CHECK(vkAllocateMemory(device->GetDevice(), &allocInfo, nullptr, &imageMemory), "Failed to allocate image memory.");

        vkBindImageMemory(device->GetDevice(), image, imageMemory, 0);

        return imageMemory;
    }

    VkImage VulkanImageBuilder::CreateImage(const VulkanDevice* device, const VkImageUsageFlags usage) const
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

        VkImage image;
        if (vkCreateImage(device->GetDevice(), &imageInfo, nullptr, &image) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create image.");
        }

        return image;
    }

    VkImageView VulkanImageBuilder::CreateImageView(const VulkanDevice* device, const VkImageAspectFlags aspectFlags) const
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        VK_CHECK_MSG(vkCreateImageView(device->GetDevice(), &viewInfo, nullptr, &imageView), "Failed to create texture image view.");

        return imageView;
    }

    VkSampler VulkanImageBuilder::CreateSampler(const VulkanDevice* device) const
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

        VkSampler sampler;

        VK_CHECK_MSG(vkCreateSampler(device->GetDevice(), &samplerInfo, nullptr, &sampler), "Failed to create texture sampler.");

        return sampler;
    }

    void VulkanImageBuilder::TransitionImageLayout(const VulkanDevice* device, const VkImageLayout oldLayout, const VkImageLayout newLayout) const
    {
        const VkCommandBuffer commandBuffer = VulkanUtils::BeginSingleTimeCommands(device->GetDevice(), device->GetCommandPool());

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = 0;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else
        {
            throw std::invalid_argument("unsupported layout transition!");
        }

        vkCmdPipelineBarrier(commandBuffer, 0, 0, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        VulkanUtils::EndSingleTimeCommands(device->GetDevice(), device->GetCommandPool(), device->GetGraphicsQueue(), commandBuffer);
    }

    void VulkanImageBuilder::CopyBufferToImage(const VulkanDevice* device, const VkBuffer buffer) const
    {
        const VkCommandBuffer commandBuffer = VulkanUtils::BeginSingleTimeCommands(device->GetDevice(), device->GetCommandPool());

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = {0, 0, 0};
        region.imageExtent = {width, height, 1};

        vkCmdCopyBufferToImage(
            commandBuffer,
            buffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
        );

        VulkanUtils::EndSingleTimeCommands(device->GetDevice(), device->GetCommandPool(), device->GetGraphicsQueue(), commandBuffer);
    }

    VulkanImage* VulkanTextureImageBuilder::Build(VulkanDevice* device)
    {
        const auto stagingBuffer = VulkanBuffer(device, size,
                                                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                                VMA_MEMORY_USAGE_CPU_ONLY);

        memcpy(stagingBuffer.GetMappedData(), data, stagingBuffer.GetBufferSize());


        image = CreateImage(device, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
        imageMemory = AllocateImageMemory(device, image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        TransitionImageLayout(device, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        CopyBufferToImage(device, stagingBuffer.GetBuffer());
        TransitionImageLayout(device, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        imageView = CreateImageView(device, VK_IMAGE_ASPECT_COLOR_BIT);
        sampler = CreateSampler(device);

        return new VulkanImage(device, image, imageMemory, imageView, sampler);
    }

    VulkanImage* VulkanDepthImageBuilder::Build(VulkanDevice* device)
    {
        format = VK_FORMAT_D24_UNORM_S8_UINT;
        tiling = VK_IMAGE_TILING_OPTIMAL;
        minFilter = VK_FILTER_LINEAR;
        magFilter = VK_FILTER_LINEAR;

        image = CreateImage(device, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
        imageMemory = AllocateImageMemory(device, image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        imageView = CreateImageView(device, VK_IMAGE_ASPECT_DEPTH_BIT);
        sampler = CreateSampler(device);

        return new VulkanImage(device, image, imageMemory, imageView, sampler);
    }
}
