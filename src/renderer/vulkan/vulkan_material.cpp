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

    VulkanMaterialBuilder& VulkanMaterialBuilder::SetBaseColorTexture(const TextureHandle& _handle)
    {
        baseColorTextureHandle = _handle;
        return *this;
    }

    VulkanMaterialBuilder& VulkanMaterialBuilder::SetNormalMapTexture(const TextureHandle& _handle)
    {
        normalMapTextureHandle = _handle;
        return *this;
    }

    VulkanMaterialBuilder& VulkanMaterialBuilder::SetMetallicRoughnessTexture(const TextureHandle& _handle)
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

        VkDescriptorSet descriptorSet;
        descriptorWriter.Build(descriptorSet);

        params.baseColor = baseColor;
        params.metallic = metallic;
        params.roughness = roughness;

        params.baseColorTextureIndex = baseColorTextureHandle.handle;
        params.normalMapTextureIndex = normalMapTextureHandle.handle;
        params.metallicRoughnessTextureIndex = metallicRoughnessTextureHandle.handle;

        params.alphaTested = isAlphaTested;

        VulkanMaterial material;
        material.index = 0;

        material.params = params;
        material.descriptorSet = descriptorSet;
        material.materialBuffer = materialBuffer;

        memcpy(material.materialBuffer->GetData(), &material.params, sizeof(MaterialParams));

        return material;
    }
}
