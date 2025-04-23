#include "vulkan_material.h"

#include "vulkan_buffer.h"
#include "vulkan_descriptor_writer.h"
#include "vulkan_device.h"
#include "vulkan_pipeline.h"
#include "resource/resource_manager.h"
#include "util/log.h"

namespace Raytracing
{
    VulkanMaterialBuilder& VulkanMaterialBuilder::SetIndex(int index)
    {
        this->index = index;
        return *this;
    }

    VulkanMaterialBuilder& VulkanMaterialBuilder::SetBaseColor(glm::vec4 baseColor)
    {
        this->baseColor = baseColor;
        return *this;
    }

    VulkanMaterialBuilder& VulkanMaterialBuilder::SetMetallic(float metallic)
    {
        this->metallic = metallic;
        return *this;
    }

    VulkanMaterialBuilder& VulkanMaterialBuilder::SetRoughness(float roughness)
    {
        this->roughness = roughness;
        return *this;
    }

    VulkanMaterialBuilder& VulkanMaterialBuilder::SetBaseColorPath(const std::string& baseColorPath)
    {
        this->baseColorPath = baseColorPath;
        return *this;
    }

    VulkanMaterialBuilder& VulkanMaterialBuilder::SetBaseColorTexture(const Ref<VulkanImage>& baseColorTexture)
    {
        this->baseColorTexture = baseColorTexture;
        return *this;
    }

    VulkanMaterialBuilder& VulkanMaterialBuilder::SetNormalMapPath(const std::string& normalMapPath)
    {
        this->normalMapPath = normalMapPath;
        return *this;
    }

    VulkanMaterialBuilder& VulkanMaterialBuilder::SetNormalMapTexture(const Ref<VulkanImage>& normalMapTexture)
    {
        this->normalMapTexture = normalMapTexture;
        return *this;
    }

    VulkanMaterialBuilder& VulkanMaterialBuilder::SetMetallicRoughnessPath(const std::string& metallicRoughnessPath)
    {
        this->metallicRoughnessPath = metallicRoughnessPath;
        return *this;
    }

    VulkanMaterialBuilder& VulkanMaterialBuilder::SetMetallicRoughnessTexture(const Ref<VulkanImage>& metallicRoughnessTexture)
    {
        this->metallicRoughnessTexture = metallicRoughnessTexture;
        return *this;
    }

    VulkanMaterialBuilder& VulkanMaterialBuilder::SetParams(const MaterialParams& params)
    {
        this->params = params;
        return *this;
    }

    VulkanMaterialBuilder& VulkanMaterialBuilder::SetPipeline(Ref<VulkanPipeline> pipeline)
    {
        this->pipeline = pipeline;
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

        auto descriptorWriter = VulkanDescriptorWriter(pipeline->GetShader()->GetVulkanDescriptorSetLayout(),
                                                       vulkanDevice->GetShaderDescriptorPool());
        descriptorWriter.WriteBuffer(0, &bufferInfo);

        if (baseColorTexture != nullptr)
        {
            params.useBaseColorMap = 1;

            VkDescriptorImageInfo baseColorImageInfo{};
            baseColorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            baseColorImageInfo.imageView = baseColorTexture->GetImageView()->Get();
            baseColorImageInfo.sampler = baseColorTexture->GetSampler();

            descriptorWriter.WriteImage(1, &baseColorImageInfo);
        }

        if (normalMapTexture != nullptr)
        {
            params.useNormalMap = 1;

            VkDescriptorImageInfo normalMapImageInfo{};
            normalMapImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            normalMapImageInfo.imageView = normalMapTexture->GetImageView()->Get();
            normalMapImageInfo.sampler = normalMapTexture->GetSampler();

            descriptorWriter.WriteImage(2, &normalMapImageInfo);
        }

        if (metallicRoughnessTexture != nullptr)
        {
            params.useMetallicRoughnessMap = 1;

            VkDescriptorImageInfo metallicRoughnessImageInfo{};
            metallicRoughnessImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            metallicRoughnessImageInfo.imageView = metallicRoughnessTexture->GetImageView()->Get();
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
        material.params = params;
        material.descriptorSet = descriptorSet;
        material.pipeline = pipeline;
        material.materialBuffer = materialBuffer;

        memcpy(material.materialBuffer->GetMappedData(), &material.params, sizeof(MaterialParams));

        return material;
    }
}
