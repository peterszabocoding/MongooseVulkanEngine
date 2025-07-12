#pragma once
#include <renderer/vulkan/vulkan_pipeline.h>

#include "../vulkan_device.h"
#include "../vulkan_framebuffer.h"
#include "../../../util/core.h"
#include "renderer/camera.h"

namespace MongooseVK
{
    enum class ResourceType: uint8_t {
        Texture = 0,
        Buffer,
    };

    enum class LoadOp: uint8_t {
        None = 0,
        Clear,
        Load,
        DontCare
    };

    enum class StoreOp: uint8_t {
        None = 0,
        Store,
        DontCare
    };

    struct PassInput {
        std::string name;
        ResourceType type;
        ImageFormat format;
    };

    struct ResourceBufferInfo {
        uint64_t size;
        AllocatedBuffer allocatedBuffer;
    };

    struct ResourceTextureInfo {
        TextureHandle textureHandle;
        TextureCreateInfo textureCreateInfo;
    };

    struct PassResource {
        std::string name;
        ResourceType type;

        ResourceBufferInfo bufferInfo;
        ResourceTextureInfo textureInfo;
    };

    struct PassOutput {
        std::string name;
        ImageFormat format;
        bool isSwapchainAttachment;
        LoadOp loadOp = LoadOp::Clear;
        StoreOp storeOp = StoreOp::Store;
        glm::vec4 clearValue;
    };

    struct RenderPassConfig {
        std::string name;
        std::vector<PassInput> inputs;
        std::vector<PassOutput> outputs;
        std::optional<PassOutput> depthStencilAttachment;

        PipelineConfig pipelineConfig;
        std::vector<VulkanDescriptorSetLayout> descriptorSetLayouts;

        RenderPassConfig& AddInput(const PassInput& input)
        {
            inputs.push_back(input);
            return *this;
        }

        RenderPassConfig& AddOutput(const PassOutput& output)
        {
            outputs.push_back(output);
            return *this;
        }

        RenderPassConfig& SetDepthStencilAttachment(const PassOutput& depthStencil)
        {
            depthStencilAttachment = depthStencil;
            return *this;
        }
    };

    class VulkanPass {
    public:
        struct Config {
            VkExtent2D imageExtent;
        };

    public:
        explicit VulkanPass(VulkanDevice* vulkanDevice, VkExtent2D _resolution): device(vulkanDevice), resolution(_resolution) {}

        virtual ~VulkanPass()
        {
            device->DestroyRenderPass(renderPassHandle);
        }

        virtual void Render(VkCommandBuffer commandBuffer,
                            Camera& camera,
                            Ref<VulkanFramebuffer> writeBuffer,
                            Ref<VulkanFramebuffer> readBuffer = nullptr) = 0;

        virtual void Resize(VkExtent2D _resolution)
        {
            resolution = _resolution;
        }

        virtual void OnResolutionChanged(const uint32_t width, const uint32_t height) {}

        virtual VulkanRenderPass* GetRenderPass() const
        {
            return device->renderPassPool.Get(renderPassHandle.handle);
        }

        void AddResource(const PassResource& resource)
        {
            resources.push_back(resource);
        }

        void ClearResources()
        {
            resources.clear();
        }

    protected:
        VulkanDevice* device;
        VkExtent2D resolution;

        RenderPassHandle renderPassHandle = {INVALID_RENDER_PASS_HANDLE};

        std::vector<PassResource> resources;
    };
}
