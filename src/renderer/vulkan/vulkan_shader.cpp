#include "vulkan_shader.h"

#include "vulkan_device.h"
#include "vulkan_utils.h"
#include "util/filesystem.h"

namespace Raytracing
{
    VulkanShader::VulkanShader(VulkanDevice *vulkanDevice, const std::string &name, const std::string &vertexCodePath,
                               const std::string &fragmentCodePath)
    {
        this->vulkanDevice = vulkanDevice;
        this->name = name;
        this->vertexCodePath = vertexCodePath;
        this->fragmentCodePath = fragmentCodePath;
    }

    VulkanShader::~VulkanShader()
    {
        for (const auto& descriptorSetLayout : descriptorSetLayouts)
            vkDestroyDescriptorSetLayout(vulkanDevice->GetDevice(), descriptorSetLayout, nullptr);
    }

    void VulkanShader::Load()
    {
        pipelineShaderStageCreateInfos.clear();
        descriptorSetLayouts.clear();

        const auto vert_shader_code = FileSystem::ReadFile(vertexCodePath);
        const VkShaderModule vert_shader_module = CreateShaderModule(vert_shader_code);

        VkPipelineShaderStageCreateInfo vert_shader_stage_info{};
        vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vert_shader_stage_info.module = vert_shader_module;
        vert_shader_stage_info.pName = "main";
        pipelineShaderStageCreateInfos.push_back(vert_shader_stage_info);
        vkDestroyShaderModule(vulkanDevice->GetDevice(), vert_shader_module, nullptr);
        const auto frag_shader_code = FileSystem::ReadFile(fragmentCodePath);
        const VkShaderModule frag_shader_module = CreateShaderModule(frag_shader_code);

        VkPipelineShaderStageCreateInfo frag_shader_stage_info{};
        frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        frag_shader_stage_info.module = frag_shader_module;
        frag_shader_stage_info.pName = "main";
        pipelineShaderStageCreateInfos.push_back(frag_shader_stage_info);
        vkDestroyShaderModule(vulkanDevice->GetDevice(), frag_shader_module, nullptr);


        descriptorSetLayouts.push_back(CreateDescriptorSetLayout());
    }

    VkShaderModule VulkanShader::CreateShaderModule(const std::vector<char> &code) const
    {
        VkShaderModuleCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        create_info.codeSize = code.size();
        create_info.pCode = reinterpret_cast<const uint32_t *>(code.data());

        VkShaderModule shader_module;
        VulkanUtils::CheckVkResult(
            vkCreateShaderModule(vulkanDevice->GetDevice(), &create_info, nullptr, &shader_module),
            "Failed to create shader module."
        );

        return shader_module;
    }

    VkDescriptorSetLayout VulkanShader::CreateDescriptorSetLayout() const
    {
        VkDescriptorSetLayout descriptorSetLayout;

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

        const std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        VulkanUtils::CheckVkResult(
            vkCreateDescriptorSetLayout(vulkanDevice->GetDevice(), &layoutInfo, nullptr, &descriptorSetLayout),
            "Failed to create descriptor set layout."
        );

        return descriptorSetLayout;
    }
}
