#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <memory/resource_pool.h>
#include <resource/resource.h>

namespace MongooseVK
{
    struct MaterialParams {
        glm::vec4 tint = {1.0f, 1.0f, 1.0f, 1.0f};
        glm::vec4 baseColor = {1.0f, 1.0f, 1.0f, 1.0f};

        float metallic = 0.0f;
        float roughness = 1.0f;

        uint32_t baseColorTextureIndex = INVALID_RESOURCE_HANDLE;
        uint32_t normalMapTextureIndex = INVALID_RESOURCE_HANDLE;
        uint32_t metallicRoughnessTextureIndex = INVALID_RESOURCE_HANDLE;

        alignas(8) uint32_t alphaTested = 0;
    };

    struct VulkanMaterial: PoolObject {
        MaterialParams params;
        AllocatedBuffer materialBuffer;
    };

    struct MaterialCreateInfo {
        glm::vec4 baseColor = {1.0f, 1.0f, 1.0f, 1.0f};
        float metallic = 0.0f;
        float roughness = 1.0f;

        TextureHandle baseColorTextureHandle = INVALID_TEXTURE_HANDLE;
        TextureHandle normalMapTextureHandle = INVALID_TEXTURE_HANDLE;
        TextureHandle metallicRoughnessTextureHandle = INVALID_TEXTURE_HANDLE;

        bool isAlphaTested = false;
    };

}
