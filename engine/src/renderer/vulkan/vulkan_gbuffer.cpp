#include "renderer/vulkan/vulkan_gbuffer.h"

#include <renderer/vulkan/vulkan_texture.h>

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

        // Viewspace Normal
        {
            const VkImageLayout imageLayout = ImageUtils::GetLayoutFromFormat(ImageFormat::RGBA32_SFLOAT);
            TextureCreateInfo textureCreateInfo{};
            textureCreateInfo.width = width;
            textureCreateInfo.height = height;
            textureCreateInfo.format = ImageFormat::RGBA32_SFLOAT;
            textureCreateInfo.imageLayout = imageLayout;
            textureCreateInfo.addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

            TextureHandle textureHandle = device->CreateTexture(textureCreateInfo);
            VulkanTexture* texture = device->texturePool.Get(textureHandle.handle);

            buffers.viewSpaceNormal.allocatedImage = texture->allocatedImage;
            buffers.viewSpaceNormal.imageView = texture->GetImageView();
            buffers.viewSpaceNormal.sampler = texture->sampler;
            buffers.viewSpaceNormal.imageLayout = imageLayout;
            buffers.viewSpaceNormal.format = VulkanUtils::ConvertImageFormat(textureCreateInfo.format);
            buffers.viewSpaceNormal.textureHandle = textureHandle;
        }

        // Viewspace Position
        {
            const VkImageLayout imageLayout = ImageUtils::GetLayoutFromFormat(ImageFormat::RGBA32_SFLOAT);
            TextureCreateInfo textureCreateInfo{};
            textureCreateInfo.width = width;
            textureCreateInfo.height = height;
            textureCreateInfo.format = ImageFormat::RGBA32_SFLOAT;
            textureCreateInfo.imageLayout = imageLayout;
            textureCreateInfo.addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

            TextureHandle textureHandle = device->CreateTexture(textureCreateInfo);
            VulkanTexture* texture = device->texturePool.Get(textureHandle.handle);

            buffers.viewSpacePosition.allocatedImage = texture->allocatedImage;
            buffers.viewSpacePosition.imageView = texture->GetImageView();
            buffers.viewSpacePosition.sampler = texture->sampler;
            buffers.viewSpacePosition.imageLayout = imageLayout;
            buffers.viewSpacePosition.format = VulkanUtils::ConvertImageFormat(textureCreateInfo.format);
            buffers.viewSpacePosition.textureHandle = textureHandle;
        }

        // Depth
        {
            const VkImageLayout imageLayout = ImageUtils::GetLayoutFromFormat(ImageFormat::DEPTH32);
            TextureCreateInfo textureCreateInfo{};
            textureCreateInfo.width = width;
            textureCreateInfo.height = height;
            textureCreateInfo.format = ImageFormat::DEPTH32;
            textureCreateInfo.imageLayout = imageLayout;
            textureCreateInfo.addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

            TextureHandle textureHandle = device->CreateTexture(textureCreateInfo);
            VulkanTexture* texture = device->texturePool.Get(textureHandle.handle);

            buffers.depth.allocatedImage = texture->allocatedImage;
            buffers.depth.imageView = texture->GetImageView();
            buffers.depth.sampler = texture->sampler;
            buffers.depth.imageLayout = imageLayout;
            buffers.depth.format = VulkanUtils::ConvertImageFormat(textureCreateInfo.format);
            buffers.depth.textureHandle = textureHandle;
        }

        return CreateRef<VulkanGBuffer>(device, width, height, buffers);
    }

    VulkanGBuffer::~VulkanGBuffer()
    {
        vulkanDevice->DestroyTexture(buffers.viewSpaceNormal.textureHandle);
        vulkanDevice->DestroyTexture(buffers.viewSpacePosition.textureHandle);
        vulkanDevice->DestroyTexture(buffers.depth.textureHandle);
    }
}
