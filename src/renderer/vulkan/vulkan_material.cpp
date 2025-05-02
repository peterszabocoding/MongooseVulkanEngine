#include "vulkan_material.h"

#include "vulkan_buffer.h"
#include "vulkan_descriptor_writer.h"
#include "vulkan_device.h"
#include "vulkan_pipeline.h"
#include "resource/resource_manager.h"
#include "util/log.h"

namespace Raytracing
{
    VulkanMaterialBuilder& VulkanMaterialBuilder::SetIndex(int _index)
    {
        index = _index;
        return *this;
    }

    VulkanMaterialBuilder& VulkanMaterialBuilder::SetBaseColor(glm::vec4 _baseColor)
    {
        baseColor = _baseColor;
        return *this;
    }

    VulkanMaterialBuilder& VulkanMaterialBuilder::SetMetallic(float _metallic)
    {
        metallic = _metallic;
        return *this;
    }

    VulkanMaterialBuilder& VulkanMaterialBuilder::SetRoughness(float _roughness)
    {
        roughness = _roughness;
        return *this;
    }

    VulkanMaterialBuilder& VulkanMaterialBuilder::SetBaseColorPath(const std::string& _baseColorPath)
    {
        baseColorPath = _baseColorPath;
        return *this;
    }

    VulkanMaterialBuilder& VulkanMaterialBuilder::SetBaseColorTexture(const Ref<VulkanTexture>& _baseColorTexture)
    {
        baseColorTexture = _baseColorTexture;
        return *this;
    }

    VulkanMaterialBuilder& VulkanMaterialBuilder::SetNormalMapPath(const std::string& _normalMapPath)
    {
        normalMapPath = _normalMapPath;
        return *this;
    }

    VulkanMaterialBuilder& VulkanMaterialBuilder::SetNormalMapTexture(const Ref<VulkanTexture>& _normalMapTexture)
    {
        normalMapTexture = _normalMapTexture;
        return *this;
    }

    VulkanMaterialBuilder& VulkanMaterialBuilder::SetMetallicRoughnessPath(const std::string& _metallicRoughnessPath)
    {
        metallicRoughnessPath = _metallicRoughnessPath;
        return *this;
    }

    VulkanMaterialBuilder& VulkanMaterialBuilder::SetMetallicRoughnessTexture(const Ref<VulkanTexture>& _metallicRoughnessTexture)
    {
        metallicRoughnessTexture = _metallicRoughnessTexture;
        return *this;
    }

    VulkanMaterialBuilder& VulkanMaterialBuilder::SetParams(const MaterialParams& _params)
    {
        params = _params;
        return *this;
    }

    VulkanMaterialBuilder& VulkanMaterialBuilder::SetDescriptorSetLayout(Ref<VulkanDescriptorSetLayout> _descriptorSetLayout)
    {
        descriptorSetLayout = _descriptorSetLayout;
        return *this;
    }

    VulkanMaterial VulkanMaterialBuilder::Build()
    {
        if (baseColorTexture == nullptr && !baseColorPath.empty())
            baseColorTexture = ResourceManager::LoadTexture(vulkanDevice, baseColorPath);

        if (normalMapTexture == nullptr && !normalMapPath.empty())
            normalMapTexture = ResourceManager::LoadTexture(vulkanDevice, normalMapPath);

        if (metallicRoughnessTexture == nullptr && !metallicRoughnessPath.empty())
            metallicRoughnessTexture = ResourceManager::LoadTexture(vulkanDevice, metallicRoughnessPath);

        Ref<VulkanBuffer> materialBuffer = CreateRef<VulkanBuffer>(
            vulkanDevice,
            sizeof(MaterialParams),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU);

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = materialBuffer->GetBuffer();
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(MaterialParams);

        auto descriptorWriter = VulkanDescriptorWriter(*descriptorSetLayout, vulkanDevice->GetShaderDescriptorPool());
        descriptorWriter.WriteBuffer(0, &bufferInfo);

        if (baseColorTexture != nullptr)
        {
            params.useBaseColorMap = 1;

            VkDescriptorImageInfo baseColorImageInfo{};
            baseColorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            baseColorImageInfo.imageView = baseColorTexture->GetImageView();
            baseColorImageInfo.sampler = baseColorTexture->GetSampler();

            descriptorWriter.WriteImage(1, &baseColorImageInfo);
        }

        if (normalMapTexture != nullptr)
        {
            params.useNormalMap = 1;

            VkDescriptorImageInfo normalMapImageInfo{};
            normalMapImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            normalMapImageInfo.imageView = normalMapTexture->GetImageView();
            normalMapImageInfo.sampler = normalMapTexture->GetSampler();

            descriptorWriter.WriteImage(2, &normalMapImageInfo);
        }

        if (metallicRoughnessTexture != nullptr)
        {
            params.useMetallicRoughnessMap = 1;

            VkDescriptorImageInfo metallicRoughnessImageInfo{};
            metallicRoughnessImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            metallicRoughnessImageInfo.imageView = metallicRoughnessTexture->GetImageView();
            metallicRoughnessImageInfo.sampler = metallicRoughnessTexture->GetSampler();

            descriptorWriter.WriteImage(3, &metallicRoughnessImageInfo);
        }

        VkDescriptorSet descriptorSet;
        descriptorWriter.Build(descriptorSet);

        params.baseColor = baseColor;
        params.metallic = metallic;
        params.roughness = roughness;

        VulkanMaterial material;
        material.index = 0;
        material.baseColorTexture = baseColorTexture;
        material.normalMapTexture = normalMapTexture;
        material.metallicRoughnessTexture = metallicRoughnessTexture;
        material.params = params;
        material.descriptorSet = descriptorSet;
        material.materialBuffer = materialBuffer;

        memcpy(material.materialBuffer->GetMappedData(), &material.params, sizeof(MaterialParams));

        return material;
    }
}
