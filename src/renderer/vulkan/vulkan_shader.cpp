#include "vulkan_shader.h"

#include <array>

#include "vulkan_descriptor_set_layout.h"
#include "vulkan_descriptor_writer.h"
#include "vulkan_utils.h"
#include "vulkan_device.h"
#include "vulkan_material.h"
#include "util/filesystem.h"

namespace Raytracing {
    VulkanShader::VulkanShader(VulkanDevice* device, const std::string& vertexShaderPath,
                               const std::string& fragmentShaderPath) {
        vulkanDevice = device;
        CreateDescriptorSetLayout();
        CreateUniformBuffer();

        Load(vertexShaderPath, fragmentShaderPath);
    }

    VulkanShader::~VulkanShader() {
        vkDestroyShaderModule(vulkanDevice->GetDevice(), vertexShaderModule, nullptr);
        vkDestroyShaderModule(vulkanDevice->GetDevice(), fragmentShaderModule, nullptr);
    }

    void VulkanShader::Load(const std::string& vertexShaderPath, const std::string& fragmentShaderPath) {
        pipelineShaderStageCreateInfos.clear();

        const auto vert_shader_code = FileSystem::ReadFile(vertexShaderPath);
        const auto frag_shader_code = FileSystem::ReadFile(fragmentShaderPath);

        vertexShaderModule = VulkanUtils::CreateShaderModule(vulkanDevice->GetDevice(), vert_shader_code);
        fragmentShaderModule = VulkanUtils::CreateShaderModule(vulkanDevice->GetDevice(), frag_shader_code);

        VkPipelineShaderStageCreateInfo vert_shader_stage_create_info{};
        vert_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vert_shader_stage_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vert_shader_stage_create_info.module = vertexShaderModule;
        vert_shader_stage_create_info.pName = "main";
        pipelineShaderStageCreateInfos.push_back(vert_shader_stage_create_info);

        VkPipelineShaderStageCreateInfo frag_shader_stage_create_info{};
        frag_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        frag_shader_stage_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        frag_shader_stage_create_info.module = fragmentShaderModule;
        frag_shader_stage_create_info.pName = "main";
        pipelineShaderStageCreateInfos.push_back(frag_shader_stage_create_info);
    }

    void VulkanShader::CreateDescriptorSetLayout() {
        vulkanDescriptorSetLayout = VulkanDescriptorSetLayout::Builder(vulkanDevice)
                .AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
                .AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // Base Color
                .AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // Normal map
                .AddBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // Metallic/Roughness
                .Build();
    }

    void VulkanShader::CreateUniformBuffer() {
        materialParamsBuffer = CreateScope<VulkanBuffer>(vulkanDevice,
                                                         sizeof(MaterialParams),
                                                         VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                                         VMA_MEMORY_USAGE_CPU_TO_GPU);
    }
}
