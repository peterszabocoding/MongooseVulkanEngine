#pragma once

#include <glm/vec3.hpp>

#include "vulkan_buffer.h"
#include "vulkan_image.h"

namespace Raytracing
{
    class VulkanShader;

    struct MaterialParams {
        glm::vec3 tint = {1.f, 1.f, 1.f};
    };

    struct VulkanMaterial {
        int index = 0;
        Ref<VulkanImage> baseColorTexture;
        MaterialParams params;
        VkDescriptorSet descriptorSet;
        Ref<VulkanBuffer> materialBuffer;
        Ref<VulkanShader> shader;
    };

    class VulkanMaterialBuilder {
    public:
        VulkanMaterialBuilder(VulkanDevice* device): vulkanDevice(device) {};
        ~VulkanMaterialBuilder() = default;

        VulkanMaterialBuilder(const VulkanMaterialBuilder&) = delete;
        VulkanMaterialBuilder& operator=(const VulkanMaterialBuilder&) = delete;

        VulkanMaterialBuilder& SetIndex(int index);
        VulkanMaterialBuilder& SetBaseColorTexture(const Ref<VulkanImage>& baseColorTexture);
        VulkanMaterialBuilder& SetParams(const MaterialParams& params);
        VulkanMaterialBuilder& SetShader(Ref<VulkanShader> shader);
        VulkanMaterial Build();

    private:
        int index = 0;
        VulkanDevice* vulkanDevice;
        Ref<VulkanImage> baseColorTexture;
        Ref<VulkanShader> shader;
        MaterialParams params;

    };
}
