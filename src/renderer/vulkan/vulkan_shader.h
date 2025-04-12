#pragma once

#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "vulkan_buffer.h"
#include "util/core.h"

namespace Raytracing
{
    class VulkanDevice;
    class VulkanImage;

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
        void SetImage(Ref<VulkanImage> vulkanImage) const;
        void UpdateUniformBuffer(const UniformBufferObject& ubo) const;

        std::vector<VkPipelineShaderStageCreateInfo>& GetPipelineShaderStageCreateInfos() { return pipelineShaderStageCreateInfos; };
        VkDescriptorSetLayout& GetDescriptorSetLayout() { return descriptorSetLayout; }
        VkDescriptorSet& GetDescriptorSet() { return descriptorSet; }

    private:
        void CreateDescriptorSetLayout();
        void CreateDescriptorSet();
        void CreateUniformBuffer();

    private:
        VulkanDevice* vulkanDevice;
        std::string name;
        std::vector<VkPipelineShaderStageCreateInfo> pipelineShaderStageCreateInfos;

        VkShaderModule vertexShaderModule{};
        VkShaderModule fragmentShaderModule{};

        VkDescriptorSetLayout descriptorSetLayout{};
        VkDescriptorSet descriptorSet{};

        VulkanBuffer* uniformBuffer;

        void* uniformBufferMapped{};
    };
}
