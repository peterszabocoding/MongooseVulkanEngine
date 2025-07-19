#pragma once

#include <array>
#include <memory>
#include <vulkan/vulkan.h>
#include <vector>
#include <unordered_map>
#include <memory/resource_pool.h>
#include <resource/resource.h>

#include "util/core.h"

namespace MongooseVK
{
    class VulkanDevice;

    enum class DescriptorSetBindingType {
        Unknown = 0,
        UniformBuffer = 1,
        TextureSampler = 2,
        StorageImage = 3,
    };

    enum class ShaderStage {
        Unknown = 0,
        VertexShader,
        FragmentShader,
        ComputeShader,
        GeometryShader,
    };

    struct DescriptorSetBinding {
        uint32_t location = 0;
        DescriptorSetBindingType type = DescriptorSetBindingType::Unknown;
        std::vector<ShaderStage> stages = {ShaderStage::Unknown};
    };

    struct DescriptorSetLayoutCreateInfo {
        std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;
        std::unordered_map<uint32_t, VkDescriptorBindingFlags> bindingFlags;
    };

    struct VulkanDescriptorSetLayout : PoolObject {
        VulkanDevice* vulkanDevice;
        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;

        uint32_t bindingCount = 0;
        std::array<VkDescriptorSetLayoutBinding, 16> bindings{};
        std::array<VkDescriptorBindingFlags, 16> bindingFlags{};
    };

    class VulkanDescriptorSetLayoutBuilder {
    public:
        explicit VulkanDescriptorSetLayoutBuilder(VulkanDevice* vulkanDevice): vulkanDevice(vulkanDevice) {}

        VulkanDescriptorSetLayoutBuilder& AddBinding(const DescriptorSetBinding& binding, uint32_t count = 1);
        DescriptorSetLayoutHandle Build() const;

    private:
        VulkanDevice* vulkanDevice;
        std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;
        std::unordered_map<uint32_t, VkDescriptorBindingFlags> bindingFlags;
    };
}
