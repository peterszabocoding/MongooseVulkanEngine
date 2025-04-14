#include "vulkan_descriptor_pool.h"

#include "vulkan_device.h"
#include "vulkan_utils.h"

namespace Raytracing
{
    // *************** Descriptor Pool Builder *********************

    VulkanDescriptorPool::Builder& VulkanDescriptorPool::Builder::AddPoolSize(
        VkDescriptorType descriptorType, uint32_t count)
    {
        poolSizes.push_back({descriptorType, count});
        return *this;
    }

    VulkanDescriptorPool::Builder& VulkanDescriptorPool::Builder::SetPoolFlags(
        VkDescriptorPoolCreateFlags flags)
    {
        poolFlags = flags;
        return *this;
    }

    VulkanDescriptorPool::Builder& VulkanDescriptorPool::Builder::SetMaxSets(uint32_t count)
    {
        maxSets = count;
        return *this;
    }

    Scope<VulkanDescriptorPool> VulkanDescriptorPool::Builder::Build()
    {
        return CreateScope<VulkanDescriptorPool>(vulkanDevice, maxSets, poolFlags, poolSizes);
    }

    // *************** Descriptor Pool *********************

    VulkanDescriptorPool::VulkanDescriptorPool(
        VulkanDevice* device,
        uint32_t maxSets,
        VkDescriptorPoolCreateFlags poolFlags,
        const std::vector<VkDescriptorPoolSize>& poolSizes)
        : vulkanDevice{device}
    {
        VkDescriptorPoolCreateInfo descriptorPoolInfo{};
        descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        descriptorPoolInfo.pPoolSizes = poolSizes.data();
        descriptorPoolInfo.maxSets = maxSets;
        descriptorPoolInfo.flags = poolFlags;

        VK_CHECK_MSG(vkCreateDescriptorPool(vulkanDevice->GetDevice(), &descriptorPoolInfo, nullptr, &descriptorPool),
                     "Failed to create descriptor pool.");
    }

    VulkanDescriptorPool::~VulkanDescriptorPool()
    {
        vkDestroyDescriptorPool(vulkanDevice->GetDevice(), descriptorPool, nullptr);
    }

    void VulkanDescriptorPool::AllocateDescriptor(
        const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptor) const
    {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.pSetLayouts = &descriptorSetLayout;
        allocInfo.descriptorSetCount = 1;

        VK_CHECK_MSG(vkAllocateDescriptorSets(vulkanDevice->GetDevice(), &allocInfo, &descriptor), "Failed to allocate descriptor sets.");
    }

    void VulkanDescriptorPool::FreeDescriptors(std::vector<VkDescriptorSet>& descriptors) const
    {
        vkFreeDescriptorSets(
            vulkanDevice->GetDevice(),
            descriptorPool,
            static_cast<uint32_t>(descriptors.size()),
            descriptors.data());
    }

    void VulkanDescriptorPool::ResetPool()
    {
        vkResetDescriptorPool(vulkanDevice->GetDevice(), descriptorPool, 0);
    }
}
