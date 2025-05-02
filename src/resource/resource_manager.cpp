#include "resource_manager.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader/tiny_obj_loader.h>

#define TINYGLTF_IMPLEMENTATION
#include <tiny_gltf/tiny_gltf.h>

#include <stdexcept>
#include <filesystem>

#include "loaders/gltf_loader.h"
#include "loaders/obj_loader.h"
#include "renderer/vulkan/vulkan_cube_map_texture.h"
#include "renderer/vulkan/vulkan_mesh.h"
#include "renderer/vulkan/vulkan_device.h"
#include "renderer/vulkan/vulkan_pipeline.h"
#include "renderer/vulkan/vulkan_texture.h"
#include "util/log.h"

namespace Raytracing
{
    ImageResource ResourceManager::LoadImageResource(const std::string& imagePath)
    {
        int width, height, channels;
        unsigned char* pixels = stbi_load(imagePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
        uint64_t size = width * height * 4;

        if (!pixels)
            throw std::runtime_error("Failed to load texture image.");

        ImageResource resource;
        resource.path = imagePath;
        resource.width = width;
        resource.height = height;
        resource.data = pixels;
        resource.size = size;
        resource.format = ImageFormat::RGB32_SFLOAT;

        return resource;
    }

    ImageResource ResourceManager::LoadHDRResource(const std::string& hdrPath)
    {
        int width, height, bitDepth;
        float* pixels = nullptr;

        //stbi_set_flip_vertically_on_load(true);
        pixels = stbi_loadf(hdrPath.c_str(), &width, &height, &bitDepth, 4);
        //stbi_set_flip_vertically_on_load(false);

        const uint64_t size = width * height * 4 * 4;

        if (!pixels)
            throw std::runtime_error("Failed to load HDR image.");

        ImageResource resource;
        resource.path = hdrPath;
        resource.width = width;
        resource.height = height;
        resource.data = pixels;
        resource.size = size;
        resource.format = ImageFormat::RGB32_SFLOAT;

        return resource;
    }

    void ResourceManager::ReleaseImage(const ImageResource& image)
    {
        LOG_TRACE("Release image data: " + image.path);
        stbi_image_free(image.data);
    }

    Ref<VulkanMesh> ResourceManager::LoadMesh(VulkanDevice* device, const std::string& meshPath)
    {
        const std::filesystem::path filePath(meshPath);

        LOG_INFO("Load Mesh: " + meshPath);

        if (filePath.extension() == ".obj")
            return ObjLoader::LoadMesh(device, meshPath);

        if (filePath.extension() == ".gltf")
            return GLTFLoader::LoadMesh(device, meshPath);


        LOG_ERROR("Failed to load mesh from file: " + filePath.string());
        abort();
    }

    Ref<VulkanMesh> ResourceManager::LoadMeshFromObj(VulkanDevice* device, const std::string& meshPath)
    {
        return ObjLoader::LoadMesh(device, meshPath);
    }

    Ref<VulkanMesh> ResourceManager::LoadMeshFromglTF(VulkanDevice* device, const std::string& meshPath)
    {
        return GLTFLoader::LoadMesh(device, meshPath);
    }

    Ref<VulkanTexture> ResourceManager::LoadTexture(VulkanDevice* device, std::string textureImagePath)
    {
        LOG_INFO("Load Texture: " + textureImagePath);
        ImageResource imageResource = LoadImageResource(textureImagePath);

        Ref<VulkanTexture> texture = VulkanTexture::Builder()
                .SetData(imageResource.data, imageResource.size)
                .SetResolution(imageResource.width, imageResource.height)
                .SetFormat(ImageFormat::RGBA8_UNORM)
                .SetFilter(VK_FILTER_LINEAR, VK_FILTER_LINEAR)
                .SetTiling(VK_IMAGE_TILING_OPTIMAL)
                .AddUsage(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                .AddUsage(VK_IMAGE_USAGE_SAMPLED_BIT)
                .AddAspectFlag(VK_IMAGE_ASPECT_COLOR_BIT)
                .SetImageResource(imageResource)
                .Build(device);

        ReleaseImage(imageResource);

        return texture;
    }

    Ref<VulkanTexture> ResourceManager::LoadHDRCubeMap(VulkanDevice* device, const std::string& hdrPath)
    {
        VkImageFormatProperties properties;
        vkGetPhysicalDeviceImageFormatProperties(device->GetPhysicalDevice(),
                                                 VK_FORMAT_R32G32B32A32_SFLOAT,
                                                 VK_IMAGE_TYPE_2D,
                                                 VK_IMAGE_TILING_OPTIMAL,
                                                 VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                                 0,
                                                 &properties);


        LOG_INFO("Load HDR: " + hdrPath);
        ImageResource imageResource = LoadHDRResource(hdrPath);
        Ref<VulkanTexture> texture = VulkanTexture::Builder()
                .SetData(imageResource.data, imageResource.size)
                .SetResolution(imageResource.width, imageResource.height)
                .SetFormat(ImageFormat::RGBA32_SFLOAT)
                .SetFilter(VK_FILTER_LINEAR, VK_FILTER_LINEAR)
                .SetTiling(VK_IMAGE_TILING_OPTIMAL)
                .AddUsage(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                .AddUsage(VK_IMAGE_USAGE_SAMPLED_BIT)
                .AddAspectFlag(VK_IMAGE_ASPECT_COLOR_BIT)
                .SetImageResource(imageResource)
                .Build(device);

        ReleaseImage(imageResource);
        return texture;
    }

    Scene ResourceManager::LoadScene(VulkanDevice* device, const std::string& scenePath)
    {
        return GLTFLoader::LoadScene(device, scenePath);
    }
}
