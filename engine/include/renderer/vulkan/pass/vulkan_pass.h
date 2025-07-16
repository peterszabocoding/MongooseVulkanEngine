#pragma once

#include "../vulkan_device.h"
#include "../vulkan_framebuffer.h"
#include "../../../util/core.h"
#include "renderer/camera.h"
#include "renderer/vulkan/vulkan_texture.h"

namespace MongooseVK
{
    enum class ResourceType: uint8_t {
        Texture = 0,
        TextureCube,
        Buffer,
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
                            Camera* camera,
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
