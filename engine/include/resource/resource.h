#pragma once

#include <vulkan/vulkan_core.h>
#include <vma/vk_mem_alloc.h>

namespace MongooseVK
{
    enum class ImageFormat {
        Unknown = 0,

        R8_UNORM,
        R8_SNORM,
        R8_UINT,
        R8_SINT,

        R16_UNORM,
        R16_SNORM,
        R16_UINT,
        R16_SINT,

        R32_SFLOAT,
        R32_UINT,
        R32_SINT,

        RGB8_UNORM,
        RGB8_SRGB,

        RGB16_UNORM,
        RGB16_SFLOAT,

        RGBA8_UNORM,
        RGBA8_SRGB,

        RGBA16_UNORM,
        RGBA16_SFLOAT,

        RGB32_SFLOAT,
        RGB32_UINT,
        RGB32_SINT,

        RGBA32_UINT,
        RGBA32_SINT,
        RGBA32_SFLOAT,

        DEPTH24_STENCIL8,
        DEPTH32,
    };

    inline bool IsDepthFormat(const ImageFormat format)
    {
        return format == ImageFormat::DEPTH24_STENCIL8 || format == ImageFormat::DEPTH32;
    }

    struct AllocatedImage {
        uint32_t width, height;
        VkImage image = VK_NULL_HANDLE;
        VkImageLayout imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        VmaAllocation allocation = VK_NULL_HANDLE;
        VmaAllocationInfo allocationInfo;
    };

    struct ImageResource {
        void* data = nullptr;
        char path[128] = "";
        uint64_t size = 0;
        uint32_t width = 0;
        uint32_t height = 0;
        uint8_t channels = 0;
        ImageFormat format = ImageFormat::Unknown;
    };

    struct TextureResource {
        void* data = nullptr;
        char path[128] = "";
        uint64_t size = 0;
        uint32_t width = 0;
        uint32_t height = 0;
        ImageFormat format = ImageFormat::Unknown;
    };

    struct AllocatedBuffer {
        VkBuffer buffer = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        VmaAllocationInfo info;
        VkDeviceAddress address;

        VkDeviceMemory GetBufferMemory() const { return info.deviceMemory; }
        VkDeviceSize GetBufferSize() const { return info.size; }
        VkDeviceSize GetOffset() const { return info.offset; }
        void* GetData() const { return info.pMappedData; }
    };

    typedef uint32_t ResourceHandle;
    static ResourceHandle INVALID_RESOURCE_HANDLE = 0xffffffff;

    struct TextureHandle {
        ResourceHandle handle;

        bool operator==(const TextureHandle& other) const { return handle == other.handle; }
        bool operator!=(const TextureHandle& other) const { return handle != other.handle; }
    };

    struct RenderPassHandle {
        ResourceHandle handle;

        bool operator==(const RenderPassHandle& other) const { return handle == other.handle; }
        bool operator!=(const RenderPassHandle& other) const { return handle != other.handle; }
    };

    struct BufferHandle {
        ResourceHandle handle;

        bool operator==(const BufferHandle& other) const { return handle == other.handle; }
        bool operator!=(const BufferHandle& other) const { return handle != other.handle; }
    };

    struct FramebufferHandle {
        ResourceHandle handle;

        bool operator==(const FramebufferHandle& other) const { return handle == other.handle; }
        bool operator!=(const FramebufferHandle& other) const { return handle != other.handle; }
    };

    struct PipelineHandle {
        ResourceHandle handle;

        bool operator==(const PipelineHandle& other) const { return handle == other.handle; }
        bool operator!=(const PipelineHandle& other) const { return handle != other.handle; }
    };

    struct DescriptorSetLayoutHandle {
        ResourceHandle handle;

        bool operator==(const DescriptorSetLayoutHandle& other) const { return handle == other.handle; }
        bool operator!=(const DescriptorSetLayoutHandle& other) const { return handle != other.handle; }
    };

    struct MaterialHandle {
        ResourceHandle handle;

        bool operator==(const MaterialHandle& other) const { return handle == other.handle; }
        bool operator!=(const MaterialHandle& other) const { return handle != other.handle; }
    };

    static TextureHandle INVALID_TEXTURE_HANDLE = {INVALID_RESOURCE_HANDLE};
    static RenderPassHandle INVALID_RENDER_PASS_HANDLE = {INVALID_RESOURCE_HANDLE};
    static BufferHandle INVALID_BUFFER_HANDLE = {INVALID_RESOURCE_HANDLE};
    static FramebufferHandle INVALID_FRAMEBUFFER_HANDLE = {INVALID_RESOURCE_HANDLE};
    static PipelineHandle INVALID_PIPELINE_HANDLE = {INVALID_RESOURCE_HANDLE};
    static DescriptorSetLayoutHandle INVALID_DESCRIPTOR_SET_LAYOUT_HANDLE = {INVALID_RESOURCE_HANDLE};
    static MaterialHandle INVALID_MATERIAL_HANDLE = {INVALID_RESOURCE_HANDLE};
}
