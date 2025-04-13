#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

#include "resource_manager.h"

#include <stdexcept>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader/tiny_obj_loader.h>

#include "renderer/vulkan/vulkan_mesh.h"
#include "renderer/vulkan/vulkan_device.h"

namespace Raytracing
{
    ImageResource ResourceManager::LoadImage(const std::string& imagePath)
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
        stbi_image_free(image.data);
    }

    VulkanMesh* ResourceManager::LoadMesh(VulkanDevice* device, const std::string& meshPath)
    {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, meshPath.c_str()))
        {
            throw std::runtime_error(warn + err);
        }

        for (const auto& shape: shapes)
        {
            for (const auto& index: shape.mesh.indices)
            {
                Vertex vertex{};

                vertex.pos = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };

                vertex.texCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                };

                vertex.color = {1.0f, 1.0f, 1.0f};

                vertices.push_back(vertex);
                indices.push_back(indices.size());
            }
        }

        return new VulkanMesh(device, vertices, indices);
    }
}
