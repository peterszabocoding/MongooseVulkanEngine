#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "vulkan_buffer.h"
#include "vulkan_descriptor_set_layout.h"

namespace MongooseVK
{
    class VulkanPipeline;
    class VulkanDevice;

    struct MaterialParams {
        glm::vec4 tint = {1.0f, 1.0f, 1.0f, 1.0f};
        glm::vec4 baseColor = {1.0f, 1.0f, 1.0f, 1.0f};

        float metallic = 0.0f;
        float roughness = 1.0f;

        uint32_t baseColorTextureIndex = INVALID_RESOURCE_HANDLE;
        uint32_t normalMapTextureIndex = INVALID_RESOURCE_HANDLE;
        uint32_t metallicRoughnessTextureIndex = INVALID_RESOURCE_HANDLE;

        int alphaTested = 0;
    };

    struct VulkanMaterial {
        int index = 0;

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

        VulkanMaterialBuilder& SetBaseColorTexture(const TextureHandle& _handle);
        VulkanMaterialBuilder& SetNormalMapTexture(const TextureHandle& _handle);
        VulkanMaterialBuilder& SetMetallicRoughnessTexture(const TextureHandle& _handle);

        VulkanMaterialBuilder& SetAlphaTested(bool _alphaTested);

        VulkanMaterialBuilder& SetParams(const MaterialParams& params);
        VulkanMaterialBuilder& SetDescriptorSetLayout(DescriptorSetLayoutHandle _descriptorSetLayout);
        VulkanMaterial Build();

    private:
        int index = 0;
        VulkanDevice* vulkanDevice;

        DescriptorSetLayoutHandle descriptorSetLayoutHandle;
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
