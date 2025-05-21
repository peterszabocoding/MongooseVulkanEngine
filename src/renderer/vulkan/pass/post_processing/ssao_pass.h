#pragma once
#include "renderer/vulkan/pass/vulkan_pass.h"

namespace Raytracing
{
    struct SSAOBuffer {
        glm::vec4 samples[64];
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

    private:
        Ref<VulkanPipeline> ssaoPipeline;
        Ref<VulkanRenderPass> renderPass{};

        SSAOBuffer buffer;
        //glm::vec3 ssaoKernel[64];
        std::vector<glm::vec4> ssaoNoiseData;

        VkDescriptorSet ssaoDescriptorSet{};
        Ref<VulkanTexture> ssaoNoiseTexture;

        Ref<VulkanBuffer> ssaoBuffer;

        Scope<VulkanMeshlet> screenRect;
    };
}
