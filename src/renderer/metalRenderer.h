#pragma once

#include "renderer.h"

#include <Metal/Metal.hpp>
#include <MetalKit/MetalKit.hpp>

#include "renderer/metalShader.h"

namespace Raytracing {
    class MetalRenderer: public Renderer {
        public:

            MetalRenderer();
            virtual ~MetalRenderer();

            virtual void Render(unsigned int image_width, unsigned int image_height) override;

            void SetMTLDevice(MTL::Device* device);
            void Draw( MTK::View* pView );

        private:
            void buildBuffers();

        private:
            MTL::Device* _pDevice;
            MTL::CommandQueue* _pCommandQueue;

            MetalShader* shader;

            MTL::Buffer* _pVertexPositionsBuffer;
            MTL::Buffer* _pVertexColorsBuffer;
            MTL::Buffer* _pIndexBuffer;
    };
}