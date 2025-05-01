#pragma once

#include <string>

namespace Raytracing {

    enum class ImageFormat {
        Unknown = 0,

        RGB8_UNORM,
        RGB8_SRGB,

        RGB16_UNORM,
        RGB16_SFLOAT,

        RGBA8_UNORM,
        RGBA8_SRGB,

        RGBA16_UNORM,
        RGBA16_SFLOAT,

        RGB32_SFLOAT,
        RGBA32_SFLOAT,

        DEPTH24_STENCIL8,
        DEPTH32,
    };


    struct ImageResource {
        std::string path = "";
        void* data = nullptr;
        uint64_t size = 0;
        uint32_t width = 0;
        uint32_t height = 0;
        ImageFormat format = ImageFormat::Unknown;
    };

    struct TextureResource {
        std::string path = "";
        void* data = nullptr;
        uint64_t size = 0;
        uint32_t width = 0;
        uint32_t height = 0;
        ImageFormat format = ImageFormat::Unknown;
    };
}