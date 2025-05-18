#pragma once
#include "renderer/vulkan/vulkan_reflection_probe.h"
#include "renderer/vulkan/vulkan_renderpass.h"

namespace Raytracing
{
    class ReflectionProbeGenerator {
    public:
        ReflectionProbeGenerator(VulkanDevice* _device);
        ~ReflectionProbeGenerator() = default;

        Ref<VulkanReflectionProbe> FromCubemap(const Ref<VulkanCubeMapTexture>& cubemap);

    private:
        void LoadPipeline();
        void GenerateBrdfLUT();
        void ComputeIblBRDF(VkCommandBuffer commandBuffer, Ref<VulkanFramebuffer> framebuffer);
        void ComputePrefilterMap(VkCommandBuffer commandBuffer, size_t faceIndex, float roughness, Ref<VulkanFramebuffer> framebuffer, VkDescriptorSet cubemapDescriptorSet);

    private:
        VulkanDevice* device = nullptr;
        Ref<VulkanTexture> brdfLUT;
        Ref<VulkanRenderPass> renderPass;

        Ref<VulkanPipeline> brdfLutPipeline;
        Ref<VulkanPipeline> prefilterPipeline;

        Scope<VulkanMeshlet> screenRect;
        Ref<VulkanMesh> cubeMesh;
    };
}
