#pragma once

#include "../vulkan_device.h"
#include "../vulkan_framebuffer.h"
#include "../../../util/core.h"
#include "renderer/camera.h"
#include "renderer/vulkan/vulkan_texture.h"

namespace MongooseVK
{
    enum class ResourceType: int8_t {
        Invalid = -1,
        Texture = 0,
        TextureCube,
        Buffer,
    };

    struct RenderPassResourceInfo {
        union {
            struct {
                uint64_t size;
                AllocatedBuffer allocatedBuffer;
            } buffer;

            struct {
                TextureHandle textureHandle;
                TextureCreateInfo textureCreateInfo;
            } texture;
        };
    };

    struct PassResource {
        std::string name;
        ResourceType type;
        RenderPassResourceInfo resourceInfo{};
    };

    struct RenderPass {
        std::string name;

        RenderPassHandle renderPass;
        FramebufferHandle framebuffer;

        std::vector<PassResource> inputs;

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

        virtual void Render(VkCommandBuffer commandBuffer, Camera* camera, FramebufferHandle writeBufferHandle) = 0;

        virtual void Resize(VkExtent2D _resolution)
        {
            resolution = _resolution;
        }

        virtual void OnResolutionChanged(const uint32_t width, const uint32_t height) {}

        virtual VulkanRenderPass* GetRenderPass() const
        {
            return device->renderPassPool.Get(renderPassHandle.handle);
        }

        virtual RenderPassHandle GetRenderPassHandle() const
        {
            return renderPassHandle;
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
