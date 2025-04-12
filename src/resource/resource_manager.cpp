#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

#include "resource_manager.h"

#include <stdexcept>

Raytracing::ImageResource Raytracing::ResourceManager::LoadImage(const std::string& imagePath)
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

void Raytracing::ResourceManager::ReleaseImage(const ImageResource& image)
{
    stbi_image_free(image.data);
}
