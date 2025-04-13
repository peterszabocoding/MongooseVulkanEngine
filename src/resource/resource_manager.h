#pragma once

#include "resource/resource.h"

namespace Raytracing {
    class ResourceManager {

    public:
        ResourceManager() = delete;

        static ImageResource LoadImage(const std::string& imagePath);
        static void ReleaseImage(const ImageResource& image);
    };
}
