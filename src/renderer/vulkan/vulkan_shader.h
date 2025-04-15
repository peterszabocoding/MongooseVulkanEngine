#pragma once

#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "vulkan_buffer.h"
#include "vulkan_descriptor_set_layout.h"
#include "vulkan_material.h"
#include "util/core.h"

namespace Raytracing
{
    class VulkanDevice;
    class VulkanImage;

    struct SimplePushConstantData {
        glm::mat4 transform{1.f};
        glm::mat4 normalMatrix{1.f};
    };

    struct UniformBufferObject {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };

    class VulkanShader {
    public:
        VulkanShader(VulkanDevice* device, const std::string& vertexShaderPath, const std::string& fragmentShaderPath);
        ~VulkanShader();

        void Load(const std::string& vertexShaderPath, const std::string& fragmentShaderPath);

        std::vector<VkPipelineShaderStageCreateInfo>& GetPipelineShaderStageCreateInfos() { return pipelineShaderStageCreateInfos; };
        VulkanDescriptorSetLayout& GetVulkanDescriptorSetLayout() { return *vulkanDescriptorSetLayout.get(); }
        VkDescriptorSetLayout& GetDescriptorSetLayout() { return vulkanDescriptorSetLayout->GetDescriptorSetLayout(); }
        VkDescriptorSet& GetDescriptorSet() { return descriptorSet; }

    private:
        void CreateDescriptorSetLayout();
        void CreateUniformBuffer();

    private:
        VulkanDevice* vulkanDevice;
        std::string name;
        std::vector<VkPipelineShaderStageCreateInfo> pipelineShaderStageCreateInfos;

        VkShaderModule vertexShaderModule{};
        VkShaderModule fragmentShaderModule{};

        VkDescriptorSet descriptorSet{};
        Scope<VulkanDescriptorSetLayout> vulkanDescriptorSetLayout;

        VulkanBuffer* materialParamsBuffer;
    };
}
