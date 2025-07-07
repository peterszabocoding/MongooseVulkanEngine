#include "vulkan_descriptor_writer.h"

#include "vulkan_descriptor_pool.h"
#include "vulkan_descriptor_set_layout.h"
#include "vulkan_device.h"
#include "util/core.h"

namespace Raytracing
{
    VulkanDescriptorWriter::VulkanDescriptorWriter(VulkanDescriptorSetLayout& setLayout, VulkanDescriptorPool& pool)
        : setLayout{setLayout}, pool{pool} {}

    VulkanDescriptorWriter& VulkanDescriptorWriter::WriteBuffer(
        const uint32_t binding, VkDescriptorBufferInfo* bufferInfo)
    {
        ASSERT(setLayout.bindings.count(binding) == 1, "Layout does not contain specified binding");

        auto& bindingDescription = setLayout.bindings[binding];

        ASSERT(
            bindingDescription.descriptorCount == 1,
            "Binding single descriptor info, but binding expects multiple");

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorType = bindingDescription.descriptorType;
        write.dstBinding = binding;
        write.pBufferInfo = bufferInfo;
        write.descriptorCount = 1;

        writes.push_back(write);
        return *this;
    }

    VulkanDescriptorWriter& VulkanDescriptorWriter::WriteImage(const uint32_t binding,
                                                               const VkDescriptorImageInfo* imageInfo,
                                                               const uint32_t arrayIndex)
    {
        ASSERT(setLayout.bindings.count(binding) == 1, "Layout does not contain specified binding");

        auto& bindingDescription = setLayout.bindings[binding];

        //ASSERT(bindingDescription.descriptorCount == 1, "Binding single descriptor info, but binding expects multiple");

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorType = bindingDescription.descriptorType;
        write.dstBinding = binding;
        write.pImageInfo = imageInfo;
        write.descriptorCount = 1;
        write.dstArrayElement = arrayIndex;

        writes.push_back(write);
        return *this;
    }

    bool VulkanDescriptorWriter::Build(VkDescriptorSet& set)
    {
        pool.AllocateDescriptor(setLayout.GetDescriptorSetLayout(), set);

        Overwrite(set);
        return true;
    }

    void VulkanDescriptorWriter::Overwrite(VkDescriptorSet& set)
    {
        for (auto& write: writes) write.dstSet = set;
        vkUpdateDescriptorSets(pool.vulkanDevice->GetDevice(), writes.size(), writes.data(), 0, nullptr);
    }

    void VulkanDescriptorWriter::BuildOrOverwrite(VkDescriptorSet& set)
    {
        if (set)
        {
            Overwrite(set);
        } else
        {
            Build(set);
        }
    }
}
