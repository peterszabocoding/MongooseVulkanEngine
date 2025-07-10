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
        Buffer
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

    struct InputResource {
        std::string name;
        ResourceType type;
        ImageFormat format;
    };

    struct OutputResource {
        std::string name;
        ImageFormat format;
        bool isSwapchainAttachment;
        LoadOp loadOp;
        StoreOp storeOp;
        glm::vec4 clearValue;
    };

    struct RenderPassConfig {
        std::string name;
        std::vector<InputResource> inputs;
        std::vector<OutputResource> outputs;
        std::optional<OutputResource> depthStencilAttachment;

        PipelineConfig pipelineConfig;
        std::vector<VulkanDescriptorSetLayout> descriptorSetLayouts;

        RenderPassConfig& AddInput(const InputResource& input) {
            inputs.push_back(input);
            return *this;
        }

        RenderPassConfig& AddOutput(const OutputResource& output) {
            outputs.push_back(output);
            return *this;
        }

        RenderPassConfig& SetDepthStencilAttachment(const OutputResource& depthStencil) {
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
        virtual ~VulkanPass() = default;

        virtual void Render(VkCommandBuffer commandBuffer,
                            Camera& camera,
                            Ref<VulkanFramebuffer> writeBuffer,
                            Ref<VulkanFramebuffer> readBuffer = nullptr) = 0;

        virtual void Resize(VkExtent2D _resolution)
        {
            resolution = _resolution;
        }

        virtual void OnResolutionChanged(const uint32_t width, const uint32_t height) {}

    protected:
        VulkanDevice* device;
        VkExtent2D resolution;
    };
}
