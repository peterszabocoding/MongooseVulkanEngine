#include "vulkan_descriptor_set_layout.h"

#include "vulkan_device.h"
#include "vulkan_utils.h"

namespace Raytracing
{
    // *************** Descriptor Set Layout Builder *********************

    VulkanDescriptorSetLayout::Builder& VulkanDescriptorSetLayout::Builder::AddBinding(
        uint32_t binding,
        VkDescriptorType descriptorType,
        VkShaderStageFlags stageFlags,
        uint32_t count)
    {
        ASSERT(bindings.count(binding) == 0, "Binding already in use");

        VkDescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.binding = binding;
        layoutBinding.descriptorType = descriptorType;
        layoutBinding.descriptorCount = count;
        layoutBinding.stageFlags = stageFlags;
        bindings[binding] = layoutBinding;
        return *this;
    }

    Ref<VulkanDescriptorSetLayout> VulkanDescriptorSetLayout::Builder::Build() const
    {
        return CreateScope<VulkanDescriptorSetLayout>(vulkanDevice, bindings);
    }

    // *************** Descriptor Set Layout *********************

    VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(
        VulkanDevice* device, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings)
        : vulkanDevice{device}, bindings{bindings}
    {
        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};
        for (auto kv: bindings)
        {
            setLayoutBindings.push_back(kv.second);
        }

        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
        descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
        descriptorSetLayoutInfo.pBindings = setLayoutBindings.data();

        VK_CHECK_MSG(vkCreateDescriptorSetLayout(vulkanDevice->GetDevice(),&descriptorSetLayoutInfo,nullptr,&descriptorSetLayout),
                     "Failed to create descriptor set layout.");
    }

    VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout()
    {
        vkDestroyDescriptorSetLayout(vulkanDevice->GetDevice(), descriptorSetLayout, nullptr);
    }
}
