#pragma once

#include <memory>
#include <vulkan/vulkan.h>
#include <vector>
#include <unordered_map>

#include "util/core.h"

namespace Raytracing
{
    class VulkanDevice;

    enum class DescriptorSetBindingType {
        Unknown = 0,
        UniformBuffer = 1,
        TextureSampler = 2,
    };

    enum class DescriptorSetShaderStage {
        Unknown = 0,
        VertexShader,
        FragmentShader,
        ComputeShader,
        GeometryShader,
    };

    struct DescriptorSetBinding {
        uint32_t location = 0;
        DescriptorSetBindingType type = DescriptorSetBindingType::Unknown;
        std::vector<DescriptorSetShaderStage> stages = { DescriptorSetShaderStage::Unknown };
    };

    class VulkanDescriptorSetLayout {
    public:
        class Builder {
        public:
            explicit Builder(VulkanDevice* vulkanDevice): vulkanDevice(vulkanDevice) {}

            Builder& AddBinding(DescriptorSetBinding binding, uint32_t count = 1);
            Ref<VulkanDescriptorSetLayout> Build() const;

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
