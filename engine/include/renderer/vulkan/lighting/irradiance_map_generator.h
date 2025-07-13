#pragma once
#include "renderer/vulkan/vulkan_renderpass.h"
#include "renderer/vulkan/vulkan_swapchain.h"
#include "util/core.h"

namespace MongooseVK
{
    class VulkanDevice;
    class VulkanCubeMapTexture;

    class IrradianceMapGenerator {
    public:
        IrradianceMapGenerator(VulkanDevice* _device);
        ~IrradianceMapGenerator();

        Ref<VulkanCubeMapTexture> FromCubemapTexture();

        VulkanRenderPass* GetRenderPass();

    private:
        void LoadPipeline();
        void ComputeIrradianceMap(VkCommandBuffer commandBuffer, size_t faceIndex, const Ref<VulkanFramebuffer>& framebuffer, VkDescriptorSet cubemapDescriptorSet);

    private:
        VulkanDevice* device;
        Ref<VulkanMesh> cubeMesh;
        RenderPassHandle renderPassHandle;
        Ref<VulkanPipeline> irradianceMapPipeline;
    };
}
