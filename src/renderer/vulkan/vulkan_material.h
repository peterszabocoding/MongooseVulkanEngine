#pragma once

#include <glm/vec3.hpp>

#include "vulkan_buffer.h"
#include "vulkan_image.h"

namespace Raytracing
{
    class VulkanPipeline;

    struct MaterialParams {
        glm::vec3 tint = {1.f, 1.f, 1.f};
        int useNormalMap = 0;
    };

    struct VulkanMaterial {
        int index = 0;
        Ref<VulkanImage> baseColorTexture;
        Ref<VulkanImage> normalMapTexture;
        Ref<VulkanImage> metallicRoughnessTexture;
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

        VulkanMaterialBuilder& SetBaseColorPath(const std::string& baseColorPath);
        VulkanMaterialBuilder& SetBaseColorTexture(const Ref<VulkanImage>& baseColorTexture);

        VulkanMaterialBuilder& SetNormalMapPath(const std::string& normalMapPath);
        VulkanMaterialBuilder& SetNormalMapTexture(const Ref<VulkanImage>& normalMapTexture);

        VulkanMaterialBuilder& SetMetallicRoughnessPath(const std::string& metallicRoughnessPath);
        VulkanMaterialBuilder& SetMetallicRoughnessTexture(const Ref<VulkanImage>& metallicRoughnessTexture);

        VulkanMaterialBuilder& SetParams(const MaterialParams& params);
        VulkanMaterialBuilder& SetPipeline(Ref<VulkanPipeline> shader);
        VulkanMaterial Build();

    private:
        int index = 0;
        VulkanDevice* vulkanDevice;

        Ref<VulkanPipeline> pipeline;
        MaterialParams params;

        Ref<VulkanImage> baseColorTexture;
        Ref<VulkanImage> normalMapTexture;
        Ref<VulkanImage> metallicRoughnessTexture;

        std::string baseColorPath;
        std::string normalMapPath;
        std::string metallicRoughnessPath;
    };
}
