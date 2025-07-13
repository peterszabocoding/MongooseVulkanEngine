#pragma once
#include "renderer/vulkan/vulkan_reflection_probe.h"
#include "renderer/vulkan/vulkan_renderpass.h"

namespace MongooseVK
{
    class ReflectionProbeGenerator {
    public:
        ReflectionProbeGenerator(VulkanDevice* _device);
        ~ReflectionProbeGenerator() = default;

        Ref<VulkanReflectionProbe> FromCubemap(TextureHandle cubemapTextureHandle);
        VulkanRenderPass* GetRenderPass();

    private:
        void LoadPipeline();
        void ComputePrefilterMap(VkCommandBuffer commandBuffer, VkExtent2D extent, size_t faceIndex, float roughness,
                                 const Ref<VulkanFramebuffer>& framebuffer, uint32_t resolution);

    private:
        VulkanDevice* device = nullptr;

        RenderPassHandle renderPassHandle;
        Ref<VulkanPipeline> prefilterPipeline;

        Scope<VulkanMeshlet> screenRect;
        Ref<VulkanMesh> cubeMesh;
    };
}
