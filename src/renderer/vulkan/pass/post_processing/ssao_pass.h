#pragma once
#include "renderer/vulkan/pass/vulkan_pass.h"

namespace Raytracing
{
    class VulkanBuffer;

    class SSAOPass : public VulkanPass {
    public:
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

        struct BlurParams {};

    public:
        explicit SSAOPass(VulkanDevice* _device, VkExtent2D _resolution);
        ~SSAOPass() override;

        virtual void Render(VkCommandBuffer commandBuffer,
                            Camera& camera,
                            Ref<VulkanFramebuffer> writeBuffer,
                            Ref<VulkanFramebuffer> readBuffer = nullptr) override;

        Ref<VulkanRenderPass> GetRenderPass() { return renderPass; }

        virtual void Resize(VkExtent2D _resolution) override;

    private:
        void LoadPipeline();
        void InitDescriptorSet();
        void GenerateNoiseData();
        void GenerateKernel();
        void BuildOutputDescriptorSet();
        void CreateFramebuffer();

    public:
        SSAOParams ssaoParams;
        Ref<VulkanFramebuffer> framebuffer;

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
