#pragma once
#include <renderer/scene.h>
#include <renderer/vulkan/vulkan_device.h>

#include "frame_graph.h"

namespace MongooseVK {

    class FrameGraphRenderPass {
    public:
        FrameGraphRenderPass(VulkanDevice* vulkanDevice, VkExtent2D _resolution): device(vulkanDevice), resolution(_resolution) {}
        virtual ~FrameGraphRenderPass() { FrameGraphRenderPass::Reset(); }

        virtual void Setup(FrameGraph* frameGraph) {};
        virtual void Init();
        virtual void Reset();
        virtual void Resize(VkExtent2D _resolution);

        virtual void PreRender(VkCommandBuffer commandBuffer, SceneGraph* scene) {}
        virtual void Render(VkCommandBuffer commandBuffer, SceneGraph* scene) {}
        virtual void OnResolutionChanged(const uint32_t width, const uint32_t height) {}

        VulkanRenderPass* GetRenderPass() const;

        void AddOutput(const FrameGraphRenderPassOutput& output);
        void AddInput(const FrameGraphResource& input);

    protected:
        virtual void LoadPipeline(PipelineCreateInfo& pipelineCreate) = 0;

        void CreatePipeline();
        virtual void CreateRenderPass();
        virtual void CreateDescriptors();
        virtual void CreateFramebuffer();

    protected:
        VulkanDevice* device;
        VkExtent2D resolution;

        PipelineHandle pipelineHandle = INVALID_PIPELINE_HANDLE;
        RenderPassHandle renderPassHandle = INVALID_RENDER_PASS_HANDLE;

        std::vector<FramebufferHandle> framebufferHandles;

        DescriptorSetLayoutHandle passDescriptorSetLayoutHandle = INVALID_DESCRIPTOR_SET_LAYOUT_HANDLE;
        VkDescriptorSet passDescriptorSet = VK_NULL_HANDLE;

        std::vector<FrameGraphResource> inputs;
        std::vector<FrameGraphRenderPassOutput> outputs;
    };

}
