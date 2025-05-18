#pragma once
#include "renderer/vulkan/vulkan_renderpass.h"
#include "renderer/vulkan/vulkan_swapchain.h"
#include "util/core.h"

namespace Raytracing
{
    class VulkanDevice;
    class VulkanCubeMapTexture;

    class IrradianceMapGenerator {
    public:
        IrradianceMapGenerator(VulkanDevice* _device);
        ~IrradianceMapGenerator() = default;

        Ref<VulkanCubeMapTexture> FromCubemapTexture(const Ref<VulkanCubeMapTexture>& cubemapTexture);

    private:
        void LoadPipeline();
        void ComputeIrradianceMap(VkCommandBuffer commandBuffer, size_t faceIndex, const Ref<VulkanFramebuffer>& framebuffer, VkDescriptorSet cubemapDescriptorSet);

    private:
        VulkanDevice* device;
        Ref<VulkanMesh> cubeMesh;
        Ref<VulkanRenderPass> renderPass;
        Ref<VulkanPipeline> irradianceMapPipeline;
    };
}
