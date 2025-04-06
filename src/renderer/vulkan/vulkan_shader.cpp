#include "vulkan_shader.h"

#include <array>

#include "vulkan_utils.h"
#include "vulkan_device.h"
#include "vulkan_image.h"
#include "util/filesystem.h"

namespace Raytracing
{
    VulkanShader::VulkanShader(VulkanDevice* device, const std::string& vertexShaderPath,
                               const std::string& fragmentShaderPath)
    {
        vulkanDevice = device;
        CreateDescriptorSetLayout();
        CreateDescriptorSet();
        CreateUniformBuffer();

        Load(vertexShaderPath, fragmentShaderPath);
    }

    VulkanShader::~VulkanShader()
    {
        delete uniformBuffer;

        vkDestroyDescriptorSetLayout(vulkanDevice->GetDevice(), descriptorSetLayout, nullptr);
        vkDestroyShaderModule(vulkanDevice->GetDevice(), vertexShaderModule, nullptr);
        vkDestroyShaderModule(vulkanDevice->GetDevice(), fragmentShaderModule, nullptr);
    }

    void VulkanShader::Load(const std::string& vertexShaderPath, const std::string& fragmentShaderPath)
    {
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

    void VulkanShader::SetImage(const VulkanImage* vulkanImage) const
    {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffer->GetBuffer();
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = vulkanImage->GetImageView();
        imageInfo.sampler = vulkanImage->GetSampler();

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSet;
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptorSet;
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(vulkanDevice->GetDevice(), descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
    }

    void VulkanShader::UpdateUniformBuffer(const UniformBufferObject& ubo) const
    {
        memcpy(uniformBufferMapped, &ubo, sizeof(ubo));
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

        VK_CHECK(vkCreateDescriptorSetLayout(vulkanDevice->GetDevice(), &layoutInfo, nullptr, &descriptorSetLayout),
                 "Failed to create descriptor set layout.");
    }

    void VulkanShader::CreateDescriptorSet()
    {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = vulkanDevice->GetDescriptorPool();
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &descriptorSetLayout;

        if (vkAllocateDescriptorSets(vulkanDevice->GetDevice(), &allocInfo, &descriptorSet) != VK_SUCCESS)
            throw std::runtime_error("failed to allocate descriptor sets!");
    }

    void VulkanShader::CreateUniformBuffer()
    {
        const VkDeviceSize buffer_size = sizeof(UniformBufferObject);

        uniformBuffer = new VulkanBuffer(
            vulkanDevice,
            sizeof(UniformBufferObject),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU);

        vkMapMemory(vulkanDevice->GetDevice(), uniformBuffer->GetBufferMemory(), 0, buffer_size, 0, &uniformBufferMapped);
    }
}
