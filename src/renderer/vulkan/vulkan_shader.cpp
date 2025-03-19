#include "vulkan_shader.h"

#include "vulkan_utils.h"
#include "util/filesystem.h"
#include "vulkan_device.h"

namespace Raytracing {
    VulkanShader::VulkanShader(VulkanDevice *device, const std::string &vertexShaderPath,
        const std::string &fragmentShaderPath) {
        vulkanDevice = device;
        CreateDescriptorSetLayout();
        Load(vertexShaderPath, fragmentShaderPath);
    }

    VulkanShader::~VulkanShader() {
        vkDestroyDescriptorSetLayout(vulkanDevice->GetDevice(), descriptorSetLayout, nullptr);
        vkDestroyShaderModule(vulkanDevice->GetDevice(), vertexShaderModule, nullptr);
        vkDestroyShaderModule(vulkanDevice->GetDevice(), fragmentShaderModule, nullptr);
    }

    void VulkanShader::Load(const std::string &vertexShaderPath, const std::string &fragmentShaderPath) {
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

    void VulkanShader::CreateDescriptorSetLayout()
    {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        VulkanUtils::CheckVkResult(
            vkCreateDescriptorSetLayout(vulkanDevice->GetDevice(), &layoutInfo, nullptr, &descriptorSetLayout),
            "Failed to create descriptor set layout."
        );
    }
}
