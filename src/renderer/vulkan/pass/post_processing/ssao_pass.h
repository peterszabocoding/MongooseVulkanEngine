#pragma once
#include "renderer/vulkan/pass/vulkan_pass.h"

namespace Raytracing
{
    struct SSAOBuffer {
        glm::vec4 samples[64];
    };

    struct SSAOParams {
        glm::vec2 resolution;
        int kernelSize = 24;
        float radius = 0.25f;
        float bias = 0.025f;
        float strength = 2.0f;
    };

    class SSAOPass : public VulkanPass {
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
        Ref<VulkanRenderPass> renderPass{};

        SSAOBuffer buffer;
        std::vector<glm::vec4> ssaoNoiseData;

        VkDescriptorSet ssaoDescriptorSet{};
        Ref<VulkanTexture> ssaoNoiseTexture;

        Ref<VulkanBuffer> ssaoBuffer;
        Scope<VulkanMeshlet> screenRect;
    };
}
