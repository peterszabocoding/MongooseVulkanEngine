#include "vulkan_material.h"

#include "vulkan_buffer.h"
#include "vulkan_descriptor_writer.h"
#include "vulkan_device.h"
#include "vulkan_pipeline.h"
#include "resource/resource_manager.h"

namespace Raytracing
{
    VulkanMaterialBuilder& VulkanMaterialBuilder::SetIndex(int index)
    {
        this->index = index;
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

        VkDescriptorImageInfo baseColorImageInfo{};
        baseColorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        baseColorImageInfo.imageView = baseColorTexture->GetImageView();
        baseColorImageInfo.sampler = baseColorTexture->GetSampler();

        VkDescriptorSet descriptorSet;

        if (normalMapTexture != nullptr && metallicRoughnessTexture != nullptr)
        {
            VkDescriptorImageInfo normalMapImageInfo{};
            normalMapImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            normalMapImageInfo.imageView = normalMapTexture->GetImageView();
            normalMapImageInfo.sampler = normalMapTexture->GetSampler();

            VkDescriptorImageInfo metallicRoughnessImageInfo{};
            metallicRoughnessImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            metallicRoughnessImageInfo.imageView = metallicRoughnessTexture->GetImageView();
            metallicRoughnessImageInfo.sampler = metallicRoughnessTexture->GetSampler();

            VulkanDescriptorWriter(pipeline->GetShader()->GetVulkanDescriptorSetLayout(), vulkanDevice->GetShaderDescriptorPool())
                .WriteBuffer(0, &bufferInfo)
                .WriteImage(1, &baseColorImageInfo)
                .WriteImage(2, &normalMapImageInfo)
                .WriteImage(3, &metallicRoughnessImageInfo)
                .Build(descriptorSet);
        } else
        {
            VulkanDescriptorWriter(pipeline->GetShader()->GetVulkanDescriptorSetLayout(), vulkanDevice->GetShaderDescriptorPool())
                .WriteBuffer(0, &bufferInfo)
                .WriteImage(1, &baseColorImageInfo)
                .Build(descriptorSet);
        }

        VulkanMaterial material;
        material.index = 0;
        material.baseColorTexture = baseColorTexture;
        material.normalMapTexture = normalMapTexture;
        material.params = params;
        material.descriptorSet = descriptorSet;
        material.pipeline = pipeline;
        material.materialBuffer = materialBuffer;

        memcpy(materialBuffer->GetMappedData(), &params, sizeof(params));

        return material;
    }
}
