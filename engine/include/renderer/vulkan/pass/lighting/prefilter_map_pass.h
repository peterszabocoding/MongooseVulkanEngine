#pragma once
#include <renderer/vulkan/pass/vulkan_pass.h>

namespace MongooseVK
{
    class PrefilterMapPass : public VulkanPass {
    public:
        PrefilterMapPass(VulkanDevice* vulkanDevice, const VkExtent2D& _resolution);

        virtual void Render(VkCommandBuffer commandBuffer, Camera* camera, Ref<VulkanFramebuffer> writeBuffer,
                            Ref<VulkanFramebuffer> readBuffer) override;

        virtual void Resize(VkExtent2D _resolution) override;

        void SetFaceIndex(uint32_t index);
        void SetRoughness(float roughness);
        void SetPassResolution(VkExtent2D _passResolution);
        void SetCubemapResolution(float _cubemapResolution);

    private:
        void LoadPipeline();

    private:
        Ref<VulkanMesh> cubeMesh;
        Ref<VulkanPipeline> prefilterMapPipeline;

        uint32_t faceIndex = 0;
        uint32_t cubemapResolution = 0;
        VkExtent2D passResolution{};
        float roughness = 0.0f;
    };
}
