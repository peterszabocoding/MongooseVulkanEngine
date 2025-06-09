#pragma once
#include "renderer/vulkan/pass/vulkan_pass.h"

namespace Raytracing
{
    class ToneMappingPass : public VulkanPass {

    public:
        explicit ToneMappingPass(VulkanDevice* _device, VkExtent2D _resolution);
        ~ToneMappingPass() override;

        virtual void Render(VkCommandBuffer commandBuffer,
                            Camera& camera,
                            Ref<VulkanFramebuffer> writeBuffer,
                            Ref<VulkanFramebuffer> readBuffer = nullptr) override;

        Ref<VulkanRenderPass> GetRenderPass() { return renderPass; }

    private:
        void LoadPipeline();
        void InitDescriptorSet();

    private:
        Ref<VulkanPipeline> pipeline;
        Ref<VulkanRenderPass> renderPass{};

        VkDescriptorSet descriptorSet{};
        Scope<VulkanMeshlet> screenRect;
    };
}
