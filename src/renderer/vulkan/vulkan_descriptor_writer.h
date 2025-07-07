#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace Raytracing
{
    class VulkanDescriptorSetLayout;
    class VulkanDescriptorPool;

    class VulkanDescriptorWriter {
    public:
        VulkanDescriptorWriter(VulkanDescriptorSetLayout& setLayout, VulkanDescriptorPool& pool);

        VulkanDescriptorWriter& WriteBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo);
        VulkanDescriptorWriter& WriteImage(uint32_t binding, const VkDescriptorImageInfo* imageInfo, uint32_t arrayIndex = 0);

        bool Build(VkDescriptorSet& set);
        void Overwrite(VkDescriptorSet& set);
        void BuildOrOverwrite(VkDescriptorSet& set);

    private:
        VulkanDescriptorSetLayout& setLayout;
        VulkanDescriptorPool& pool;
        std::vector<VkWriteDescriptorSet> writes;
    };
}
