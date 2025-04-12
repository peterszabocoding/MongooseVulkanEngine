#pragma once

#include <string>

namespace Raytracing {
    enum class ImageFormat
    {
        None = 0,
        RED8UN,
        RED8UI,
        RED16UI,
        RED32UI,
        RED32F,
        RG8,
        RG16F,
        RG32F,
        RGB,
        RGBA,
        RGBA16F,
        RGBA32F,
        B10R11G11UF,
        SRGB,
    };

    struct ImageResource {
        std::string path = "";
        unsigned char* data = nullptr;
        uint64_t size = 0;
        uint32_t width = 0;
        uint32_t height = 0;
        ImageFormat format = ImageFormat::None;
    };
}