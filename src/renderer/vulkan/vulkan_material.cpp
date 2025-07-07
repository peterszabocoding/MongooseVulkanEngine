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

    VulkanMaterialBuilder& VulkanMaterialBuilder::SetBaseColorTextureHandle(const TextureHandle& _handle)
    {
        baseColorTextureHandle = _handle;
        return *this;
    }

    VulkanMaterialBuilder& VulkanMaterialBuilder::SetNormalMapPath(const std::string& _normalMapPath)
    {
        normalMapPath = _normalMapPath;
        return *this;
    }

    VulkanMaterialBuilder& VulkanMaterialBuilder::SetNormalMapTextureHandle(const TextureHandle& _handle)
    {
        normalMapTextureHandle = _handle;
        return *this;
    }

    VulkanMaterialBuilder& VulkanMaterialBuilder::SetMetallicRoughnessPath(const std::string& _metallicRoughnessPath)
    {
        metallicRoughnessPath = _metallicRoughnessPath;
        return *this;
    }

    VulkanMaterialBuilder& VulkanMaterialBuilder::SetMetallicRoughnessTextureHandle(const TextureHandle& _handle)
    {
        metallicRoughnessTextureHandle = _handle;
        return *this;
    }

    VulkanMaterialBuilder& VulkanMaterialBuilder::SetAlphaTested(bool _alphaTested) {
        isAlphaTested = _alphaTested;
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
        const Ref<VulkanBuffer> materialBuffer = CreateRef<VulkanBuffer>(
            vulkanDevice,
            sizeof(MaterialParams),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU);

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = materialBuffer->GetBuffer();
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(MaterialParams);

        auto descriptorWriter = VulkanDescriptorWriter(*descriptorSetLayout, vulkanDevice->GetShaderDescriptorPool());
        descriptorWriter.WriteBuffer(0, &bufferInfo);

        if (baseColorTextureHandle != INVALID_TEXTURE_HANDLE)
        {
            params.useBaseColorMap = 1;
            const VulkanTexture* texture = vulkanDevice->texturePool.Get(baseColorTextureHandle.handle);

            VkDescriptorImageInfo baseColorImageInfo{};
            baseColorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            baseColorImageInfo.imageView = texture->GetImageView();
            baseColorImageInfo.sampler = texture->GetSampler();

            descriptorWriter.WriteImage(1, &baseColorImageInfo);
        }

        if (normalMapTextureHandle != INVALID_TEXTURE_HANDLE)
        {
            params.useNormalMap = 1;
            const VulkanTexture* texture = vulkanDevice->texturePool.Get(normalMapTextureHandle.handle);

            VkDescriptorImageInfo normalMapImageInfo{};
            normalMapImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            normalMapImageInfo.imageView = texture->GetImageView();
            normalMapImageInfo.sampler = texture->GetSampler();

            descriptorWriter.WriteImage(2, &normalMapImageInfo);
        }

        if (metallicRoughnessTextureHandle != INVALID_TEXTURE_HANDLE)
        {
            params.useMetallicRoughnessMap = 1;
            const VulkanTexture* texture = vulkanDevice->texturePool.Get(metallicRoughnessTextureHandle.handle);

            VkDescriptorImageInfo metallicRoughnessImageInfo{};
            metallicRoughnessImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            metallicRoughnessImageInfo.imageView = texture->GetImageView();
            metallicRoughnessImageInfo.sampler = texture->GetSampler();

            descriptorWriter.WriteImage(3, &metallicRoughnessImageInfo);
        }

        VkDescriptorSet descriptorSet;
        descriptorWriter.Build(descriptorSet);

        params.baseColor = baseColor;
        params.metallic = metallic;
        params.roughness = roughness;

        params.alphaTested = isAlphaTested;

        VulkanMaterial material;
        material.index = 0;

        material.baseColorTextureHandle = baseColorTextureHandle;
        material.normalMapTextureHandle = normalMapTextureHandle;
        material.metallicRoughnessTextureHandle = metallicRoughnessTextureHandle;

        material.params = params;
        material.descriptorSet = descriptorSet;
        material.materialBuffer = materialBuffer;

        memcpy(material.materialBuffer->GetMappedData(), &material.params, sizeof(MaterialParams));

        return material;
    }
}
