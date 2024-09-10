#pragma once

#include "math/vec3.h"

namespace Raytracing {

    struct Vertex {
        vec3 position;
        vec3 texCoord;
        vec3 color = vec3();
    };

    class Renderer {

        public:
            virtual ~Renderer() {};

            virtual void Render() = 0;
            virtual void SetResolution(unsigned long image_width, unsigned long image_height) = 0;
    };
}