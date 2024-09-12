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
            
            virtual void Render();
            virtual void SetResolution(unsigned long image_width, unsigned long image_height);

        protected:
            virtual void ProcessPixel(unsigned int pixelCount, vec3 pixelColor) = 0;
            virtual void OnRenderFinished() = 0;

        protected:
            unsigned long renderWidth = 0, renderHeight = 0;

    };
}