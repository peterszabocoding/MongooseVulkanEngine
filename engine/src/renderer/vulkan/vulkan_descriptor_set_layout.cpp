#include "renderer/vulkan/vulkan_descriptor_set_layout.h"

#include "renderer/vulkan/vulkan_device.h"
#include "renderer/vulkan/vulkan_utils.h"

namespace MongooseVK
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
                case DescriptorSetBindingType::StorageImage:
                    return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

                default:
                    ASSERT(false, "Unknown descriptor type");
                    return VK_DESCRIPTOR_TYPE_MAX_ENUM;
            }
        }

        static VkShaderStageFlagBits ConvertShaderStage(const ShaderStage type)
        {
            switch (type)
            {
                case ShaderStage::VertexShader:
                    return VK_SHADER_STAGE_VERTEX_BIT;

                case ShaderStage::FragmentShader:
                    return VK_SHADER_STAGE_FRAGMENT_BIT;

                case ShaderStage::GeometryShader:
                    return VK_SHADER_STAGE_GEOMETRY_BIT;

                case ShaderStage::ComputeShader:
                    return VK_SHADER_STAGE_COMPUTE_BIT;

                default:
                    ASSERT(false, "Unknown shader stage");
            }

            return VK_SHADER_STAGE_ALL;
        }
    }

    // *************** Descriptor Set Layout Builder *********************
    VulkanDescriptorSetLayoutBuilder& VulkanDescriptorSetLayoutBuilder::AddBinding(
        const DescriptorSetBinding& binding, const uint32_t count)
    {
        ASSERT(bindings.contains(binding.location) == 0, "Binding already in use");

        VkShaderStageFlags stageFlags = 0;

        for (auto& stage: binding.stages)
            stageFlags |= Utils::ConvertShaderStage(stage);

        VkDescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.binding = binding.location;
        layoutBinding.descriptorType = Utils::ConvertDescriptorType(binding.type);
        layoutBinding.descriptorCount = count;
        layoutBinding.stageFlags = stageFlags;

        bindings[binding.location] = layoutBinding;
        bindingFlags[binding.location] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;

        return *this;
    }

    DescriptorSetLayoutHandle VulkanDescriptorSetLayoutBuilder::Build() const
    {
        DescriptorSetLayoutCreateInfo createInfo{bindings, bindingFlags};
        return vulkanDevice->CreateDescriptorSetLayout(createInfo);
    }

}
