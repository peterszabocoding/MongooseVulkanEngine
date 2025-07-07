#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "vulkan_buffer.h"
#include "vulkan_device.h"
#include "vulkan_texture.h"
#include "vulkan_descriptor_set_layout.h"

namespace Raytracing
{
    class VulkanPipeline;

    struct MaterialParams {
        glm::vec4 tint = {1.0f, 1.0f, 1.0f, 1.0f};
        glm::vec4 baseColor = {1.0f, 1.0f, 1.0f, 1.0f};

        float metallic = 0.0f;
        float roughness = 1.0f;

        int useBaseColorMap = 0;
        int useNormalMap = 0;
        int useMetallicRoughnessMap = 0;
        int alphaTested = 0;
    };

    struct VulkanMaterial {
        int index = 0;

        TextureHandle baseColorTextureHandle = INVALID_TEXTURE_HANDLE;
        TextureHandle normalMapTextureHandle = INVALID_TEXTURE_HANDLE;
        TextureHandle metallicRoughnessTextureHandle = INVALID_TEXTURE_HANDLE;

        MaterialParams params;
        VkDescriptorSet descriptorSet;
        Ref<VulkanBuffer> materialBuffer;
    };

    class VulkanMaterialBuilder {
    public:
        explicit VulkanMaterialBuilder(VulkanDevice* device): vulkanDevice(device) {};
        ~VulkanMaterialBuilder() = default;

        VulkanMaterialBuilder(const VulkanMaterialBuilder&) = delete;
        VulkanMaterialBuilder& operator=(const VulkanMaterialBuilder&) = delete;

        VulkanMaterialBuilder& SetIndex(int index);

        VulkanMaterialBuilder& SetBaseColor(glm::vec4 baseColor);
        VulkanMaterialBuilder& SetMetallic(float metallic);
        VulkanMaterialBuilder& SetRoughness(float roughness);

        VulkanMaterialBuilder& SetBaseColorPath(const std::string& baseColorPath);
        VulkanMaterialBuilder& SetBaseColorTextureHandle(const TextureHandle& _handle);

        VulkanMaterialBuilder& SetNormalMapPath(const std::string& normalMapPath);
        VulkanMaterialBuilder& SetNormalMapTextureHandle(const TextureHandle& _handle);

        VulkanMaterialBuilder& SetMetallicRoughnessPath(const std::string& metallicRoughnessPath);
        VulkanMaterialBuilder& SetMetallicRoughnessTextureHandle(const TextureHandle& _handle);

        VulkanMaterialBuilder& SetAlphaTested(bool _alphaTested);

        VulkanMaterialBuilder& SetParams(const MaterialParams& params);
        VulkanMaterialBuilder& SetDescriptorSetLayout(Ref<VulkanDescriptorSetLayout> _descriptorSetLayout);
        VulkanMaterial Build();

    private:
        int index = 0;
        VulkanDevice* vulkanDevice;

        Ref<VulkanDescriptorSetLayout> descriptorSetLayout;
        MaterialParams params;

        TextureHandle baseColorTextureHandle = INVALID_TEXTURE_HANDLE;
        TextureHandle normalMapTextureHandle = INVALID_TEXTURE_HANDLE;
        TextureHandle metallicRoughnessTextureHandle = INVALID_TEXTURE_HANDLE;

        std::string baseColorPath;
        std::string normalMapPath;
        std::string metallicRoughnessPath;

        glm::vec4 baseColor = {1.0f, 1.0f, 1.0f, 1.0f};
        float metallic = 0.0f;
        float roughness = 1.0f;

        bool isAlphaTested = false;
    };
}
