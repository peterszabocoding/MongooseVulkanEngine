#include "resource/resource_manager.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader/tiny_obj_loader.h>

#define TINYGLTF_IMPLEMENTATION
#include <tiny_gltf/tiny_gltf.h>

#include <stdexcept>
#include <filesystem>

#include "resource/loaders/gltf_loader.h"
#include "resource/loaders/obj_loader.h"
#include "renderer/bitmap.h"
#include "renderer/vulkan/vulkan_mesh.h"
#include "renderer/vulkan/vulkan_device.h"
#include "renderer/vulkan/vulkan_pipeline.h"
#include "renderer/vulkan/vulkan_texture.h"
#include "util/log.h"

namespace MongooseVK
{
    ImageResource ResourceManager::LoadImageResource(const std::string& imagePath)
    {
        int width, height, channels;
        unsigned char* pixels = stbi_load(imagePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
        const uint64_t size = width * height * 4;

        if (!pixels)
            throw std::runtime_error("Failed to load texture image.");

        ImageResource resource;
        resource.width = width;
        resource.height = height;
        resource.data = pixels;
        resource.size = size;
        resource.channels = channels;
        resource.format = ImageFormat::RGBA8_UNORM;
        std::copy(imagePath.begin(), imagePath.end(), resource.path);

        return resource;
    }

    ImageResource ResourceManager::LoadHDRResource(const std::string& hdrPath)
    {
        int width, height, bitDepth;
        float* pixels = nullptr;

        pixels = stbi_loadf(hdrPath.c_str(), &width, &height, &bitDepth, 4);

        const uint64_t size = width * height * 4 * 4;

        if (!pixels)
            throw std::runtime_error("Failed to load HDR image.");

        ImageResource resource;
        resource.width = width;
        resource.height = height;
        resource.data = pixels;
        resource.size = size;
        resource.format = ImageFormat::RGB32_SFLOAT;
        std::copy(hdrPath.begin(), hdrPath.end(), resource.path);

        return resource;
    }

    void ResourceManager::ReleaseImage(const ImageResource& image)
    {
        LOG_TRACE("Release image data: {0}", image.path);
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

    Ref<VulkanTexture> ResourceManager::LoadTexture(VulkanDevice* device, const std::string& textureImagePath)
    {
        LOG_INFO("Load Texture: " + textureImagePath);
        ImageResource imageResource = LoadImageResource(textureImagePath);

        const uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(imageResource.width, imageResource.height)))) + 1;
        Ref<VulkanTexture> texture = VulkanTextureBuilder()
                .SetData(imageResource.data, imageResource.size)
                .SetResolution(imageResource.width, imageResource.height)
                .SetFormat(imageResource.format)
                .SetFilter(VK_FILTER_LINEAR, VK_FILTER_LINEAR)
                .SetTiling(VK_IMAGE_TILING_OPTIMAL)
                .AddUsage(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                .AddUsage(VK_IMAGE_USAGE_SAMPLED_BIT)
                .AddAspectFlag(VK_IMAGE_ASPECT_COLOR_BIT)
                .SetImageResource(imageResource)
                .SetMipLevels(mipLevels)
                .Build(device);

        ReleaseImage(imageResource);

        return texture;
    }

    TextureHandle ResourceManager::LoadTexture2(VulkanDevice* device, const std::string& textureImagePath)
    {
        LOG_INFO("Load Texture: " + textureImagePath);
        const ImageResource imageResource = LoadImageResource(textureImagePath);
        const TextureHandle textureHandle = device->CreateTexture({
            .width = imageResource.width,
            .height = imageResource.height,
            .format = imageResource.format,
            .data = imageResource.data,
            .size = imageResource.size,
            .generateMipMaps = true,
        });
        ReleaseImage(imageResource);

        return textureHandle;
    }

    Bitmap ResourceManager::LoadHDRCubeMapBitmap(VulkanDevice* device, const std::string& hdrPath)
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
        const ImageResource imageResource = LoadHDRResource(hdrPath);

        const Bitmap in(imageResource.width, imageResource.height, 4, eBitmapFormat_Float, imageResource.data);
        const Bitmap verticalCross = Bitmap::ConvertEquirectangularMapToVerticalCross(in);
        Bitmap cubeMapBitmap = Bitmap::ConvertVerticalCrossToCubeMapFaces(verticalCross);

        return cubeMapBitmap;
    }

    void ResourceManager::LoadAndSaveHDR(const std::string& hdrPath)
    {
        const ImageResource imageResource = LoadHDRResource(hdrPath);
        const Bitmap in(imageResource.width, imageResource.height, 4, eBitmapFormat_Float, imageResource.data);
        const Bitmap verticalCross = Bitmap::ConvertEquirectangularMapToVerticalCross(in);
        const Bitmap* out = new Bitmap(Bitmap::ConvertVerticalCrossToCubeMapFaces(verticalCross));

        ReleaseImage(imageResource);

        stbi_write_hdr("./cache/verticalCross.hdr", verticalCross.width, verticalCross.height, verticalCross.comp,
                       (const float*) verticalCross.pixelData.data());

        for (int i = 0; i < 6; i++)
        {
            const uint64_t offset = Bitmap::GetImageOffsetForFace(*out, i);
            stbi_write_hdr(("./cache/face_" + std::to_string(i) + ".hdr").c_str(), out->width, out->height, out->comp,
                           reinterpret_cast<const float*>(out->pixelData.data()) + offset);
        }

        delete out;
    }

    Scene ResourceManager::LoadScene(VulkanDevice* device, const std::string& scenePath, const std::string& skyboxPath)
    {
        Scene scene = GLTFLoader::LoadScene(device, scenePath);
        scene.scenePath = scenePath;
        scene.skyboxPath = skyboxPath;

        return scene;
    }
}
