#include "vulkan_texture_image.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stdexcept>
#include <stb_image/stb_image.h>

#include "vulkan_buffer.h"
#include "vulkan_utils.h"
#include "vulkan_device.h"

namespace Raytracing
{
    VulkanTextureImage::VulkanTextureImage(VulkanDevice *device, const std::string &image_path): VulkanImage(device)
    {
        CreateTextureImage(image_path);
        CreateTextureImageView(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
        CreateTextureSampler();
    }

    void VulkanTextureImage::CreateTextureImage(const std::string &image_path)
    {
        int tex_width, tex_height, tex_channels;
        stbi_uc *pixels = stbi_load(image_path.c_str(), &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);
        VkDeviceSize imageSize = tex_width * tex_height * 4;

        if (!pixels)
            throw std::runtime_error("Failed to load texture image.");

        const auto stagingBuffer = VulkanBuffer(
            device,
            imageSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            VMA_MEMORY_USAGE_CPU_ONLY);

        void *data = stagingBuffer.GetMappedData();
        memcpy(data, pixels, stagingBuffer.GetBufferSize());

        stbi_image_free(pixels);

        CreateImage(
            tex_width,
            tex_height,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            textureImage,
            textureImageMemory
            );

        TransitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        CopyBufferToImage(stagingBuffer.GetBuffer(), textureImage, static_cast<uint32_t>(tex_width), static_cast<uint32_t>(tex_height));
        TransitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
}

