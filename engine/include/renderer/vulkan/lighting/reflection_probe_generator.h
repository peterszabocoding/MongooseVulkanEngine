#pragma once
#include "renderer/vulkan/vulkan_reflection_probe.h"
#include "renderer/vulkan/vulkan_renderpass.h"

namespace MongooseVK
{
    class ReflectionProbeGenerator {
    public:
        ReflectionProbeGenerator(VulkanDevice* _device);
        ~ReflectionProbeGenerator() = default;

        Ref<VulkanReflectionProbe> FromCubemap(const Ref<VulkanCubeMapTexture>& cubemap);

        VulkanRenderPass* GetRenderPass();

    private:
        void LoadPipeline();
        void GenerateBrdfLUT();
        void ComputeIblBRDF(VkCommandBuffer commandBuffer, const Ref<VulkanFramebuffer>& framebuffer);
        void ComputePrefilterMap(VkCommandBuffer commandBuffer, VkExtent2D extent, size_t faceIndex, float roughness,
                                 const Ref<VulkanFramebuffer>& framebuffer, VkDescriptorSet cubemapDescriptorSet, uint32_t resolution);

    private:
        VulkanDevice* device = nullptr;
        Ref<VulkanTexture> brdfLUT;
        RenderPassHandle renderPassHandle;

        Ref<VulkanPipeline> brdfLutPipeline;
        Ref<VulkanPipeline> prefilterPipeline;

        Scope<VulkanMeshlet> screenRect;
        Ref<VulkanMesh> cubeMesh;
    };
}
