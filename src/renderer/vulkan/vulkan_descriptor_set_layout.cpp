#include "vulkan_descriptor_set_layout.h"

#include "vulkan_device.h"
#include "vulkan_utils.h"

namespace Raytracing
{
    namespace Utils
    {
        static VkDescriptorType ConvertDescriptorType(const DescriptorSetBindingType type)
        {
            switch (type)
            {
                case DescriptorSetBindingType::UniformBuffer:
                    return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                case DescriptorSetBindingType::TextureSampler:
                    return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

                default:
                    ASSERT(false, "Unknown descriptor type");
            }
        }

        static VkShaderStageFlagBits ConvertShaderStage(const DescriptorSetShaderStage type)
        {
            switch (type)
            {
                case DescriptorSetShaderStage::VertexShader:
                    return VK_SHADER_STAGE_VERTEX_BIT;

                case DescriptorSetShaderStage::FragmentShader:
                    return VK_SHADER_STAGE_FRAGMENT_BIT;

                case DescriptorSetShaderStage::GeometryShader:
                    return VK_SHADER_STAGE_GEOMETRY_BIT;

                case DescriptorSetShaderStage::ComputeShader:
                    return VK_SHADER_STAGE_COMPUTE_BIT;

                default:
                    ASSERT(false, "Unknown shader stage");
            }

            return VK_SHADER_STAGE_ALL;
        }
    }

    // *************** Descriptor Set Layout Builder *********************
    VulkanDescriptorSetLayout::Builder& VulkanDescriptorSetLayout::Builder::AddBinding(
        const DescriptorSetBinding binding, const uint32_t count)
    {
        ASSERT(bindings.count(binding) == 0, "Binding already in use");

        VkShaderStageFlags stageFlags = 0;

        for (auto& stage: binding.stages)
            stageFlags |= Utils::ConvertShaderStage(stage);

        VkDescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.binding = binding.location;
        layoutBinding.descriptorType = Utils::ConvertDescriptorType(binding.type);
        layoutBinding.descriptorCount = count;
        layoutBinding.stageFlags = stageFlags;

        bindings[binding.location] = layoutBinding;
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
