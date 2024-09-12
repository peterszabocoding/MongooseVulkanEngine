#pragma once

#include "renderer.h"
#include "math/vec3.h"

#include <Metal/Metal.hpp>
#include <MetalKit/MetalKit.hpp>

#include "renderer/metalShader.h"

#include <simd/simd.h>

namespace Raytracing {

    struct MetalVertex {
        simd::float3 position;
        simd::float2 texCoord;
    };

    class MetalRenderer: public Renderer {
        public:
            MetalRenderer();
            virtual ~MetalRenderer();
            virtual void SetResolution(unsigned long image_width, unsigned long image_height) override;

            void SetMTLDevice(MTL::Device* device);
            void Draw( MTK::View* pView );

        protected:
            virtual void ProcessPixel(unsigned int pixelCount, vec3 pixelColor) override;
            virtual void OnRenderFinished() override;

        private:
            void buildBuffers();

        private:
            simd::uint1* image;

            MTL::Device* _pDevice;
            MTL::CommandQueue* _pCommandQueue;

            MetalShader* shader;

            MTL::Buffer* _pVertexPositionsBuffer;
            MTL::Buffer* _pVertexColorsBuffer;

            MTL::Buffer* _pVertexBuffer;
            MTL::Buffer* _pIndexBuffer;

            MTL::Texture* _pTexture;
            MTL::Region region;
    };
}