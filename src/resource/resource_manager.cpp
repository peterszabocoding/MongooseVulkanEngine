#include "resource_manager.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader/tiny_obj_loader.h>

#define TINYGLTF_IMPLEMENTATION
#include <iostream>
#include <tiny_gltf/tiny_gltf.h>

#include <stdexcept>
#include <filesystem>

#include "loaders/gltf_loader.h"
#include "loaders/obj_loader.h"
#include "renderer/vulkan/vulkan_mesh.h"
#include "renderer/vulkan/vulkan_device.h"
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
        resource.format = ImageFormat::RGBA32F;

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

    Ref<VulkanImage> ResourceManager::LoadTexture(VulkanDevice* device, std::string textureImagePath)
    {
        LOG_TRACE("Load Texture: " + textureImagePath);

        const ImageResource imageResource = LoadImageResource(textureImagePath);

        VulkanTextureImageBuilder textureImageBuilder;
        textureImageBuilder.SetData(imageResource.data, imageResource.size);
        textureImageBuilder.SetResolution(imageResource.width, imageResource.height);
        textureImageBuilder.SetFormat(VK_FORMAT_R8G8B8A8_UNORM);
        textureImageBuilder.SetFilter(VK_FILTER_LINEAR, VK_FILTER_LINEAR);
        textureImageBuilder.SetTiling(VK_IMAGE_TILING_OPTIMAL);

        Ref<VulkanImage> texture = textureImageBuilder.Build(device);

        ReleaseImage(imageResource);

        return texture;
    }
}
