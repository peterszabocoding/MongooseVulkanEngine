#pragma once
#include "renderer/vulkan/pass/vulkan_pass.h"

namespace Raytracing
{
    class SSAOPass : public VulkanPass {
    public:
        struct SSAOBuffer {
            glm::vec4 samples[64];
        };

        struct SSAOParams {
            glm::vec2 resolution;
            int kernelSize = 16;
            float radius = 0.5f;
            float bias = 0.075f;
            float strength = 2.0f;
        };

        struct BlurParams {};

    public:
        explicit SSAOPass(VulkanDevice* _device);
        ~SSAOPass() override;

        void Update();

        virtual void Render(VkCommandBuffer commandBuffer, uint32_t imageIndex, Ref<VulkanFramebuffer> writeBuffer,
                            Ref<VulkanFramebuffer> readBuffer) override;

        Ref<VulkanRenderPass> GetRenderPass() { return renderPass; }

    private:
        void LoadPipeline();
        void InitDescriptorSet();
        void GenerateNoiseData();
        void GenerateKernel();

    public:
        SSAOParams ssaoParams;

    private:
        Ref<VulkanPipeline> ssaoPipeline;
        Ref<VulkanPipeline> blurPipeline;
        Ref<VulkanRenderPass> renderPass{};

        SSAOBuffer buffer;
        std::vector<glm::vec4> ssaoNoiseData;

        VkDescriptorSet ssaoDescriptorSet{};
        VkDescriptorSet boxBlurDescriptorSet;

        Ref<VulkanTexture> ssaoNoiseTexture;

        Ref<VulkanBuffer> ssaoBuffer;
        Scope<VulkanMeshlet> screenRect;
    };
}
