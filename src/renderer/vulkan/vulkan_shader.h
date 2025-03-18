#pragma once

#include <string>
#include <vulkan/vulkan.h>
#include <vector>

namespace Raytracing
{
    class VulkanDevice;

    class VulkanShader {
    public:
        VulkanShader(VulkanDevice *vulkanDevice, const std::string &name, const std::string &vertexCodePath,
                     const std::string &fragmentCodePath);
        ~VulkanShader();

        void Load();

        std::vector<VkDescriptorSetLayout>& GetDescriptorSetLayouts() { return descriptorSetLayouts; }
        std::vector<VkPipelineShaderStageCreateInfo>& GetPipelineShaderStageCreateInfos() { return pipelineShaderStageCreateInfos; }

    private:
        VkShaderModule CreateShaderModule(const std::vector<char> &code) const;
        VkDescriptorSetLayout CreateDescriptorSetLayout() const;

    private:
        VulkanDevice *vulkanDevice;

        std::string name;
        std::string vertexCodePath;
        std::string fragmentCodePath;
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
        std::vector<VkPipelineShaderStageCreateInfo> pipelineShaderStageCreateInfos;
        VkDescriptorSet descriptorSet{};
    };
}
