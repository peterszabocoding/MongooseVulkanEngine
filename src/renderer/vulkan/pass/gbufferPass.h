#pragma once
#include "vulkan_pass.h"
#include "renderer/render_graph.h"
#include "renderer/scene.h"
#include "renderer/vulkan/vulkan_gbuffer.h"

namespace Raytracing

{
    class GBufferPass : public VulkanPass {
    public:
        explicit GBufferPass(VulkanDevice* vulkanDevice, Scene& _scene, VkExtent2D _resolution);
        virtual ~GBufferPass() override = default;

        virtual void Render(VkCommandBuffer commandBuffer,
                            Camera& camera,
                            Ref<VulkanFramebuffer> writeBuffer,
                            Ref<VulkanFramebuffer> readBuffer = nullptr) override;

        void Resize(VkExtent2D _resolution) override;

        Ref<VulkanRenderPass> GetRenderPass() { return renderPass; }

        void ExecuteWithRenderGraph(VkCommandBuffer cmd, const std::unordered_map<std::string, RenderResource*>& resources);

    private:
        void LoadPipelines();
        void CreateFramebuffer();
        void BuildGBuffer();

    public:
        Ref<VulkanGBuffer> gBuffer;
        Ref<VulkanFramebuffer> framebuffer{};

    private:
        Scene& scene;

        Ref<VulkanRenderPass> renderPass{};
        Ref<VulkanPipeline> gbufferPipeline;
    };
}
