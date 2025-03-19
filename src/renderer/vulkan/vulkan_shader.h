#pragma once

#include <string>
#include <vector>
#include <vulkan/vulkan.h>

namespace Raytracing {
    class VulkanDevice;

    class VulkanShader {
    public:
        VulkanShader(VulkanDevice *device, const std::string &vertexShaderPath, const std::string &fragmentShaderPath);

        ~VulkanShader();

        void Load(const std::string &vertexShaderPath, const std::string &fragmentShaderPath);
        std::vector<VkPipelineShaderStageCreateInfo>& GetPipelineShaderStageCreateInfos() { return pipelineShaderStageCreateInfos; };
        VkDescriptorSetLayout& GetDescriptorSetLayout() { return descriptorSetLayout; }

    private:
        void CreateDescriptorSetLayout();

    private:
        VulkanDevice* vulkanDevice;
        std::string name;
        std::vector<VkPipelineShaderStageCreateInfo> pipelineShaderStageCreateInfos;

        VkShaderModule vertexShaderModule;
        VkShaderModule fragmentShaderModule;

        VkDescriptorSetLayout descriptorSetLayout;
    };
}
