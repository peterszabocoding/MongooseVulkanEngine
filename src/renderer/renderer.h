#pragma once

namespace Raytracing {
    class Renderer {

        public:
            virtual ~Renderer() {};

            virtual void Render(unsigned int image_width, unsigned int image_height) = 0;

    };
}