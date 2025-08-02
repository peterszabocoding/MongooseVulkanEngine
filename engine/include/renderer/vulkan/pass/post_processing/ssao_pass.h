#pragma once
#include "renderer/vulkan/pass/vulkan_pass.h"

namespace MongooseVK
{
    class VulkanBuffer;

    class SSAOPass final : public VulkanPass {
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

        virtual void InitDescriptors() override;
        virtual void Render(VkCommandBuffer commandBuffer, Camera* camera, FramebufferHandle writeBuffer) override;
        virtual void Resize(VkExtent2D _resolution) override;

    protected:
        virtual void LoadPipeline() override;

    private:
        void InitDescriptorSet();
        void GenerateNoiseData();
        void GenerateKernel();

    public:
        SSAOParams ssaoParams;

    private:
        SSAOBuffer buffer;
        std::vector<glm::vec4> ssaoNoiseData;

        DescriptorSetLayoutHandle ssaoDescriptorSetLayout;
        VkDescriptorSet ssaoDescriptorSet{};

        TextureHandle ssaoNoiseTextureHandle;

        Ref<VulkanBuffer> ssaoBuffer;
        Scope<VulkanMeshlet> screenRect;
    };
}
