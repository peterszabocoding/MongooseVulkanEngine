#pragma once

#include <vector>
#include <vulkan/vulkan.h>

#include "util/core.h"

namespace MongooseVK
{
    class VulkanDevice;

    class VulkanDescriptorPool {
    public:
        class Builder {
        public:
            Builder(VulkanDevice* device): vulkanDevice(device) {}

            Builder& AddPoolSize(VkDescriptorType descriptorType, uint32_t count);
            Builder& SetPoolFlags(VkDescriptorPoolCreateFlags flags);
            Builder& SetMaxSets(uint32_t count);
            Scope<VulkanDescriptorPool> Build();

        private:
            VulkanDevice* vulkanDevice;
            std::vector<VkDescriptorPoolSize> poolSizes;
            uint32_t maxSets = 1000;
            VkDescriptorPoolCreateFlags poolFlags = 0;
        };

    public:
        VulkanDescriptorPool(VulkanDevice* vulkanDevice, uint32_t maxSets, VkDescriptorPoolCreateFlags poolFlags,
                             const std::vector<VkDescriptorPoolSize>& poolSizes);
        ~VulkanDescriptorPool();
        VulkanDescriptorPool(const VulkanDescriptorPool&) = delete;
        VulkanDescriptorPool& operator=(const VulkanDescriptorPool&) = delete;

        void AllocateDescriptor(const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptorSet) const;
        void FreeDescriptors(std::vector<VkDescriptorSet>& descriptors) const;
        void ResetPool();

        VkDescriptorPool GetDescriptorPool() const { return descriptorPool; }

    private:
        VulkanDevice* vulkanDevice;
        VkDescriptorPool descriptorPool;

        friend class VulkanDescriptorWriter;
    };
}
