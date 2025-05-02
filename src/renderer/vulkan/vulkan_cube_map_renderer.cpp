//
// Created by peter on 2025. 04. 28..
//

#include "vulkan_cube_map_renderer.h"

#include "vulkan_descriptor_writer.h"
#include "vulkan_framebuffer.h"
#include "vulkan_pipeline.h"

namespace Raytracing
{
    glm::mat4 VulkanCubeMapRenderer::captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    std::array<glm::mat4, 6> VulkanCubeMapRenderer::captureViews = {
        lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
        lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))
    };

    VulkanCubeMapRenderer::VulkanCubeMapRenderer(VulkanDevice* device, Ref<VulkanRenderPass> renderPass)
    {
        descriptorSetLayout = VulkanDescriptorSetLayout::Builder(device)
                .AddBinding({0, DescriptorSetBindingType::UniformBuffer, {DescriptorSetShaderStage::VertexShader}})
                .AddBinding({1, DescriptorSetBindingType::TextureSampler, {DescriptorSetShaderStage::FragmentShader}})
                .Build();

        pipeline = VulkanPipeline::Builder()
                .SetShaders("shader/spv/equirectangularToCubemap.vert.spv", "shader/spv/equirectangularToCubemap.frag.spv")
                .SetDescriptorSetLayout(descriptorSetLayout)
                .SetCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
                .SetPolygonMode(VK_POLYGON_MODE_FILL)
                .DisableDepthTest()
                .DisableBlending()
                .AddColorAttachment(ImageFormat::RGBA16_SFLOAT)
                .SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                .SetMultisampling(VK_SAMPLE_COUNT_1_BIT)
                .SetRenderpass(renderPass)
                .Build(device);

        for (size_t i = 0; i < 6; i++)
        {
            projectionViewBuffers[i] = CreateRef<VulkanBuffer>(
                device,
                sizeof(ProjectionViewBuffer),
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                VMA_MEMORY_USAGE_CPU_TO_GPU);
        }
    }

    void VulkanCubeMapRenderer::Load(VulkanDevice* device, Ref<VulkanTexture> hdrTexture)
    {
        VkDescriptorImageInfo hdrImageInfo{};
        hdrImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        hdrImageInfo.imageView = hdrTexture->GetImageView();
        hdrImageInfo.sampler = hdrTexture->GetSampler();

        for (size_t i = 0; i < 6; i++)
        {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = projectionViewBuffers[i]->GetBuffer();
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(ProjectionViewBuffer);

            auto writer = VulkanDescriptorWriter(*descriptorSetLayout, device->GetShaderDescriptorPool());
            writer.WriteBuffer(0, &bufferInfo);
            writer.WriteImage(1, &hdrImageInfo);
            writer.Build(descriptorSets[i]);

            ProjectionViewBuffer buffer;
            buffer.projection = captureProjection;
            buffer.view = captureViews[i];

            memcpy(projectionViewBuffers[i]->GetMappedData(), &buffer, sizeof(ProjectionViewBuffer));
        }
    }
}
