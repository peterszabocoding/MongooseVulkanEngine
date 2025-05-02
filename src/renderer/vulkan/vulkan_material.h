#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "vulkan_buffer.h"
#include "vulkan_descriptor_set_layout.h"
#include "vulkan_image.h"
#include "vulkan_texture.h"

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
    };

    struct VulkanMaterial {
        int index = 0;
        Ref<VulkanTexture> baseColorTexture;
        Ref<VulkanTexture> normalMapTexture;
        Ref<VulkanTexture> metallicRoughnessTexture;
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
        VulkanMaterialBuilder& SetBaseColorTexture(const Ref<VulkanTexture>& baseColorTexture);

        VulkanMaterialBuilder& SetNormalMapPath(const std::string& normalMapPath);
        VulkanMaterialBuilder& SetNormalMapTexture(const Ref<VulkanTexture>& normalMapTexture);

        VulkanMaterialBuilder& SetMetallicRoughnessPath(const std::string& metallicRoughnessPath);
        VulkanMaterialBuilder& SetMetallicRoughnessTexture(const Ref<VulkanTexture>& metallicRoughnessTexture);

        VulkanMaterialBuilder& SetParams(const MaterialParams& params);
        VulkanMaterialBuilder& SetDescriptorSetLayout(Ref<VulkanDescriptorSetLayout> _descriptorSetLayout);
        VulkanMaterial Build();

    private:
        int index = 0;
        VulkanDevice* vulkanDevice;

        Ref<VulkanDescriptorSetLayout> descriptorSetLayout;
        MaterialParams params;

        Ref<VulkanTexture> baseColorTexture;
        Ref<VulkanTexture> normalMapTexture;
        Ref<VulkanTexture> metallicRoughnessTexture;

        std::string baseColorPath;
        std::string normalMapPath;
        std::string metallicRoughnessPath;

        glm::vec4 baseColor = {1.0f, 1.0f, 1.0f, 1.0f};
        float metallic = 0.0f;
        float roughness = 1.0f;
    };
}
