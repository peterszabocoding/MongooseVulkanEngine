#pragma once

#define GLM_FORCE_RADIANS
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

#include "vulkan_cube_map_texture.h"
#include "vulkan_descriptor_set_layout.h"

namespace Raytracing
{
    struct ProjectionViewBuffer {
        glm::mat4 projection;
        glm::mat4 view;
    };

    class VulkanCubeMapRenderer {
    public:
        VulkanCubeMapRenderer(VulkanDevice* device, Ref<VulkanRenderPass> renderPass);
        ~VulkanCubeMapRenderer() {}

        void Load(Ref<VulkanTexture> hdrTexture);

    public:
        static glm::mat4 captureProjection;
        static std::array<glm::mat4, 6> captureViews;

        VulkanDevice* device;

        Ref<VulkanPipeline> pipeline;

        Ref<VulkanDescriptorSetLayout> descriptorSetLayout;
        std::array<Ref<VulkanBuffer>, 6> projectionViewBuffers;
        std::array<VkDescriptorSet, 6> descriptorSets;
    };
}
