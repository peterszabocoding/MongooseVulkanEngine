#pragma once
#include "util/core.h"
#include "vulkan/vulkan_buffer.h"
#include "vulkan/vulkan_descriptor_set_layout.h"
#include "vulkan/vulkan_pipeline.h"

namespace Raytracing
{
    struct DescriptorSetLayouts {
        Ref<VulkanDescriptorSetLayout> skyboxDescriptorSetLayout;
        Ref<VulkanDescriptorSetLayout> materialDescriptorSetLayout;
        Ref<VulkanDescriptorSetLayout> transformDescriptorSetLayout;
        Ref<VulkanDescriptorSetLayout> lightsDescriptorSetLayout;
        Ref<VulkanDescriptorSetLayout> presentDescriptorSetLayout;
    };

    struct DescriptorSets {
        VkDescriptorSet skyboxDescriptorSet;
        VkDescriptorSet transformDescriptorSet;
        std::vector<VkDescriptorSet> presentDescriptorSets;
        std::vector<VkDescriptorSet> lightsDescriptorSets;
    };

    struct Pipelines {
        Ref<VulkanPipeline> skyBox;
        Ref<VulkanPipeline> geometry;
        Ref<VulkanPipeline> directionalShadowMap;
        Ref<VulkanPipeline> present;
        Ref<VulkanPipeline> ibl_brdf;
    };

    struct Renderpass {
        Ref<VulkanRenderPass> skyboxPass{};
        Ref<VulkanRenderPass> geometryPass{};
        Ref<VulkanRenderPass> shadowMapPass{};
        Ref<VulkanRenderPass> presentPass{};
        Ref<VulkanRenderPass> iblBrdfPass{};
    };

    class ShaderCache {
    public:
        ShaderCache(VulkanDevice* _vulkanDevice): vulkanDevice(_vulkanDevice) {}
        ~ShaderCache() = default;

        ShaderCache(const ShaderCache& other) = delete;
        ShaderCache(ShaderCache&& other) = delete;

        void Load();

    public:
        static Pipelines pipelines;
        static DescriptorSetLayouts descriptorSetLayouts;
        static DescriptorSets descriptorSets;
        static Renderpass renderpasses;

    private:
        void LoadDescriptorLayouts();
        void LoadPipelines();
        void LoadRenderpasses();
    private:
        VulkanDevice* vulkanDevice;
    };
}
