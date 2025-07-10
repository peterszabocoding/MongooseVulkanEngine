#pragma once
#include "vulkan_pass.h"
#include "renderer/scene.h"

namespace MongooseVK
{
    class SkyboxPass : public VulkanPass {
    public:
        SkyboxPass(VulkanDevice* vulkanDevice, Scene& _scene, VkExtent2D _resolution);
        ~SkyboxPass() override = default;

        virtual void Render(VkCommandBuffer commandBuffer,
                            Camera& camera,
                            Ref<VulkanFramebuffer> writeBuffer,
                            Ref<VulkanFramebuffer> readBuffer = nullptr) override;

        VulkanRenderPass* GetRenderPass();

    private:
        void LoadPipelines();

    private:
        Scene& scene;

        RenderPassHandle renderPassHandle;
        Ref<VulkanPipeline> skyboxPipeline;
        Ref<VulkanMesh> cubeMesh;
    };
} // Raytracing
