#pragma once

#include "renderer.h"

#include <Metal/Metal.hpp>
#include <MetalKit/MetalKit.hpp>

namespace Raytracing {
    class MetalRenderer: public Renderer {
        public:

            MetalRenderer();
            virtual ~MetalRenderer();

            virtual void Render(unsigned int image_width, unsigned int image_height) override;


            void SetMTLDevice(MTL::Device* device);
            void draw( MTK::View* pView );

        private:
            MTL::Device* _pDevice;
            MTL::CommandQueue* _pCommandQueue;
    };
}