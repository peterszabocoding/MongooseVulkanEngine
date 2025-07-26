#pragma once

#include <array>
#include <vulkan/vulkan.h>
#include <vector>

namespace MongooseVK
{
    class VulkanDescriptorSetLayout;
    class VulkanDescriptorPool;

    class VulkanDescriptorWriter {
    public:
        VulkanDescriptorWriter(VulkanDescriptorSetLayout& setLayout, VulkanDescriptorPool& pool);

        VulkanDescriptorWriter& WriteBuffer(uint32_t binding, VkDescriptorBufferInfo bufferInfo);
        VulkanDescriptorWriter& WriteImage(uint32_t binding, VkDescriptorImageInfo imageInfo, uint32_t arrayIndex = 0);

        bool Build(VkDescriptorSet& set);
        void Overwrite(VkDescriptorSet& set);
        void BuildOrOverwrite(VkDescriptorSet& set);

    private:
        VulkanDescriptorSetLayout& setLayout;
        VulkanDescriptorPool& pool;

        uint32_t bufferInfoCount = 0;
        std::array<VkDescriptorBufferInfo, 32> bufferInfos;

        uint32_t imageInfoCount = 0;
        std::array<VkDescriptorImageInfo, 32> imageInfos;

        std::vector<VkWriteDescriptorSet> writes;
    };
}
