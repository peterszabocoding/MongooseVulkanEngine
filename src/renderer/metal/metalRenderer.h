#pragma once

#include <Metal/Metal.hpp>
#include <MetalKit/MetalKit.hpp>
#include <simd/simd.h>

#include "metalShader.h"
#include "renderer/renderer.h"

namespace Raytracing {

    struct MetalVertex {
        simd::float3 position;
        simd::float2 texCoord;
    };

    class MetalRenderer: public Renderer {
        public:
            MetalRenderer();
            virtual ~MetalRenderer();

            void SetMTLDevice(MTL::Device* device);
            void Draw( MTK::View* pView );

            virtual void Init(int width, int height) override {};

        protected:
            virtual void ProcessPixel(unsigned int pixelCount, vec3 pixelColor) override;

            virtual void OnRenderBegin(const Camera& camera) override;
            virtual void OnRenderFinished(const Camera& camera) override;

        private:
            void buildBuffers();
            void createTexture(unsigned int width, unsigned int height);

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