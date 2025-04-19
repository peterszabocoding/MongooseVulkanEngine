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
        baseColorTexture = ResourceManager::LoadTexture(vulkanDevice, baseColorPath);

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

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = baseColorTexture->GetImageView();
        imageInfo.sampler = baseColorTexture->GetSampler();

        VkDescriptorSet descriptorSet;
        VulkanDescriptorWriter(pipeline->GetShader()->GetVulkanDescriptorSetLayout(), vulkanDevice->GetShaderDescriptorPool())
                .WriteBuffer(0, &bufferInfo)
                .WriteImage(1, &imageInfo)
                .Build(descriptorSet);

        VulkanMaterial material;
        material.index = 0;
        material.baseColorTexture = baseColorTexture;
        material.params = params;
        material.descriptorSet = descriptorSet;
        material.pipeline = pipeline;
        material.materialBuffer = materialBuffer;

        memcpy(materialBuffer->GetMappedData(), &params, sizeof(params));

        return material;
    }
}
