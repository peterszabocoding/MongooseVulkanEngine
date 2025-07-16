#pragma once

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

    static TextureHandle INVALID_TEXTURE_HANDLE = {INVALID_RESOURCE_HANDLE};
    static RenderPassHandle INVALID_RENDER_PASS_HANDLE = {INVALID_RESOURCE_HANDLE};
    static BufferHandle INVALID_BUFFER_HANDLE = {INVALID_RESOURCE_HANDLE};
    static FramebufferHandle INVALID_FRAMEBUFFER_HANDLE = {INVALID_RESOURCE_HANDLE};
}
