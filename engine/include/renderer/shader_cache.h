#pragma once
#include "util/core.h"
#include "vulkan/vulkan_descriptor_set_layout.h"
#include "vulkan/vulkan_pipeline.h"

namespace MongooseVK
{
    constexpr auto SHADER_PATH = "shader/glsl/";

    struct DescriptorSetLayouts {
        Ref<VulkanDescriptorSetLayout> cubemapDescriptorSetLayout;
        Ref<VulkanDescriptorSetLayout> materialDescriptorSetLayout;
        Ref<VulkanDescriptorSetLayout> cameraDescriptorSetLayout;
        Ref<VulkanDescriptorSetLayout> lightsDescriptorSetLayout;
        Ref<VulkanDescriptorSetLayout> directionalShadowMapDescriptorSetLayout;
        Ref<VulkanDescriptorSetLayout> presentDescriptorSetLayout;
        Ref<VulkanDescriptorSetLayout> irradianceDescriptorSetLayout;
        Ref<VulkanDescriptorSetLayout> reflectionDescriptorSetLayout;
        Ref<VulkanDescriptorSetLayout> viewspaceNormalDescriptorSetLayout;
        Ref<VulkanDescriptorSetLayout> viewspacePositionDescriptorSetLayout;
        Ref<VulkanDescriptorSetLayout> depthMapDescriptorSetLayout;
        Ref<VulkanDescriptorSetLayout> ssaoDescriptorSetLayout;
        Ref<VulkanDescriptorSetLayout> postProcessingDescriptorSetLayout;
        Ref<VulkanDescriptorSetLayout> ppBoxBlurDescriptorSetLayout;
        Ref<VulkanDescriptorSetLayout> toneMappingDescriptorSetLayout;
    };

    struct DescriptorSets {
        VkDescriptorSet cubemapDescriptorSet;
        VkDescriptorSet cameraDescriptorSet;
        VkDescriptorSet irradianceDescriptorSet;
        VkDescriptorSet reflectionDescriptorSet;
        VkDescriptorSet lightsDescriptorSet;
        VkDescriptorSet directionalShadownMapDescriptorSet;
        VkDescriptorSet viewspaceNormalDescriptorSet;
        VkDescriptorSet viewspacePositionDescriptorSet;
        VkDescriptorSet depthMapDescriptorSet;
        VkDescriptorSet postProcessingDescriptorSet;
        VkDescriptorSet toneMappingDescriptorSet;
        VkDescriptorSet presentDescriptorSet;
    };

    class ShaderCache {
    public:
        ShaderCache(VulkanDevice* _vulkanDevice): vulkanDevice(_vulkanDevice) { Load(); }
        ~ShaderCache() = default;

        ShaderCache(const ShaderCache& other) = delete;
        ShaderCache(ShaderCache&& other) = delete;

        void Load();

    public:
        static DescriptorSetLayouts descriptorSetLayouts;
        static DescriptorSets descriptorSets;
        static std::unordered_map<std::string, std::vector<uint32_t>> shaderCache;

    private:
        void LoadShaders();
        void LoadDescriptorLayouts();
        void LoadPipelines();
        void LoadRenderpasses();

    private:
        VulkanDevice* vulkanDevice;
    };
}
