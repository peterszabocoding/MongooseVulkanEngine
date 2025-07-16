#pragma once
#include <renderer/vulkan/pass/vulkan_pass.h>
#include <renderer/vulkan/vulkan_mesh.h>

namespace MongooseVK
{
    class BrdfLUTPass final : public VulkanPass {
    public:
        BrdfLUTPass(VulkanDevice* vulkanDevice, VkExtent2D _resolution);
        ~BrdfLUTPass() override = default;

        virtual void Render(VkCommandBuffer commandBuffer, Camera* camera, FramebufferHandle writeBufferHandle) override;
        virtual void Resize(VkExtent2D _resolution) override;

    private:
        void LoadPipeline();

    private:
        Ref<VulkanPipeline> brdfLUTPipeline;
        Ref<VulkanMeshlet> screenRect;
    };
}
