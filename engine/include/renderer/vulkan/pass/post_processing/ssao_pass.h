#pragma once
#include <renderer/frame_graph.h>

namespace MongooseVK
{
    class VulkanBuffer;

    class SSAOPass final : public FrameGraphRenderPass {
    public:
        struct SSAOBuffer {
            glm::vec4 samples[64];
        };

        struct SSAOParams {
            glm::vec2 resolution;
            int kernelSize = 45;
            float radius = 0.15f;
            float bias = 0.005f;
            float strength = 1.0f;
        };

        struct BlurParams {};

    public:
        explicit SSAOPass(VulkanDevice* _device, VkExtent2D _resolution);
        ~SSAOPass() override;

        virtual void CreateDescriptors() override;
        virtual void Render(VkCommandBuffer commandBuffer, SceneGraph* scene) override;
        virtual void Resize(VkExtent2D _resolution) override;

    protected:
        virtual void LoadPipeline(PipelineCreateInfo& pipelineCreate) override;

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
        Scope<VulkanMesh> screenRect;
    };
}
