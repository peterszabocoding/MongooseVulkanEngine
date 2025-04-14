#pragma once

#include <memory>
#include <vulkan/vulkan.h>
#include <vector>
#include <unordered_map>

#include "util/core.h"

namespace Raytracing
{
    class VulkanDevice;

    class VulkanDescriptorSetLayout {
    public:
        class Builder {
        public:
            explicit Builder(VulkanDevice* vulkanDevice): vulkanDevice(vulkanDevice) {}

            Builder& AddBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t count = 1);
            Scope<VulkanDescriptorSetLayout> Build() const;

        private:
            VulkanDevice* vulkanDevice;
            std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;
        };

    public:
        VulkanDescriptorSetLayout(VulkanDevice* vulkanDevice, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings);
        ~VulkanDescriptorSetLayout();
        VulkanDescriptorSetLayout(const VulkanDescriptorSetLayout&) = delete;
        VulkanDescriptorSetLayout& operator=(const VulkanDescriptorSetLayout&) = delete;

        VkDescriptorSetLayout& GetDescriptorSetLayout() { return descriptorSetLayout; }

    private:
        VulkanDevice* vulkanDevice;
        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
        std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;

        friend class VulkanDescriptorWriter;
    };
}
