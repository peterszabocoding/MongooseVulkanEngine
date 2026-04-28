#pragma once
#include "renderer/frame_graph/frame_graph_renderpass.h"

namespace MongooseVK
{
    class VulkanBuffer;

    class SSAOPass final : public FrameGraph::FrameGraphRenderPass {
    public:
        struct SSAOBuffer {
            glm::vec4 samples[64];
            alignas(16)glm::vec2 resolution;
            int kernelSize = 45;
            float radius = 0.15f;
            float bias = 0.005f;
            float strength = 1.0f;
        };

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
        SSAOBuffer ssaoParams;

        std::vector<glm::vec4> ssaoNoiseData;

        DescriptorSetLayoutHandle ssaoDescriptorSetLayout;
        VkDescriptorSet ssaoDescriptorSet{};

        TextureHandle ssaoNoiseTextureHandle;

        AllocatedBuffer ssaoBuffer;
        Scope<VulkanMesh> screenRect;
    };
}
