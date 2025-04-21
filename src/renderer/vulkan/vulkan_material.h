#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "vulkan_buffer.h"
#include "vulkan_image.h"

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
        Ref<VulkanTextureImage> baseColorTexture;
        Ref<VulkanTextureImage> normalMapTexture;
        Ref<VulkanTextureImage> metallicRoughnessTexture;
        MaterialParams params;
        VkDescriptorSet descriptorSet;
        Ref<VulkanBuffer> materialBuffer;
        Ref<VulkanPipeline> pipeline;
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
        VulkanMaterialBuilder& SetBaseColorTexture(const Ref<VulkanTextureImage>& baseColorTexture);

        VulkanMaterialBuilder& SetNormalMapPath(const std::string& normalMapPath);
        VulkanMaterialBuilder& SetNormalMapTexture(const Ref<VulkanTextureImage>& normalMapTexture);

        VulkanMaterialBuilder& SetMetallicRoughnessPath(const std::string& metallicRoughnessPath);
        VulkanMaterialBuilder& SetMetallicRoughnessTexture(const Ref<VulkanTextureImage>& metallicRoughnessTexture);

        VulkanMaterialBuilder& SetParams(const MaterialParams& params);
        VulkanMaterialBuilder& SetPipeline(Ref<VulkanPipeline> shader);
        VulkanMaterial Build();

    private:
        int index = 0;
        VulkanDevice* vulkanDevice;

        Ref<VulkanPipeline> pipeline;
        MaterialParams params;

        Ref<VulkanTextureImage> baseColorTexture;
        Ref<VulkanTextureImage> normalMapTexture;
        Ref<VulkanTextureImage> metallicRoughnessTexture;

        std::string baseColorPath;
        std::string normalMapPath;
        std::string metallicRoughnessPath;

        glm::vec4 baseColor = {1.0f, 1.0f, 1.0f, 1.0f};
        float metallic = 0.0f;
        float roughness = 1.0f;
    };
}
