#include "metalRenderer.h"
#include <iostream>

#include "math/color.h"
#include "math/vec3.h"

#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define MTK_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#include <Metal/Metal.hpp>
#include <AppKit/AppKit.hpp>
#include <MetalKit/MetalKit.hpp>

#include <simd/simd.h>

namespace Raytracing {

    MetalRenderer::MetalRenderer()
    {
    }

    MetalRenderer::~MetalRenderer()
    {
        _pCommandQueue->release();
        _pDevice->release();
    }

    void MetalRenderer::Render(unsigned int image_width, unsigned int image_height)
    {
        // Render
        std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

        for (int j = 0; j < image_height; j++) {
            std::clog << "\rScanlines remaining: " << (image_height - j) << ' ' << std::flush;

            for (int i = 0; i < image_width; i++) {
                color pixelColor(
                    double(i) / (image_width-1), 
                    double(j) / (image_height-1), 
                    1.0);
                write_color(std::cout, pixelColor);
            }
        }

        std::clog << "\rDone.                 \n";
    }

    void MetalRenderer::SetMTLDevice(MTL::Device *device)
    {
        _pDevice = device;
        _pCommandQueue = _pDevice->newCommandQueue();

        shader = new MetalShader(_pDevice, "shader/shader.metal");

        buildBuffers();
    }

    void MetalRenderer::Draw(MTK::View *pView)
    {
        NS::AutoreleasePool* pPool = NS::AutoreleasePool::alloc()->init();

        MTL::CommandBuffer* pCmd = _pCommandQueue->commandBuffer();
        MTL::RenderPassDescriptor* pRpd = pView->currentRenderPassDescriptor();
        MTL::RenderCommandEncoder* pEnc = pCmd->renderCommandEncoder( pRpd );

        pEnc->setRenderPipelineState( shader->GetRenderPipelineState() );
        pEnc->setVertexBuffer( _pVertexPositionsBuffer, 0, 0 );
        pEnc->setVertexBuffer( _pVertexColorsBuffer, 0, 1 );
        //pEnc->drawPrimitives( MTL::PrimitiveType::PrimitiveTypeTriangle, NS::UInteger(0), NS::UInteger(3) );

        pEnc->drawIndexedPrimitives(
            MTL::PrimitiveTypeTriangleStrip, 
            _pIndexBuffer->length(), 
            MTL::IndexType::IndexTypeUInt16, 
            _pIndexBuffer, 
            0);

        pEnc->endEncoding();
        pCmd->presentDrawable( pView->currentDrawable() );
        pCmd->commit();

        pPool->release();
    }

    void MetalRenderer::buildBuffers()
    {
        const size_t NumVertices = 4;
        const size_t Indices = 6;

        simd::float3 positions[NumVertices] =
        {
            { -1.0f, -1.0f, 0.0f },
            { -1.0f, 1.0f, 0.0f },
            { 1.0f,  1.0f, 0.0f },
            { 1.0f,  -1.0f, 0.0f },
        };

        simd::float3 colors[NumVertices] =
        {
            {  0.0,     0.0f,   0.0f },
            {  1.0f,    0.0f,   0.0f },
            {  1.0f,    1.0f,   0.0f },
            {  0.0f,    1.0f,   0.0f }
        };

        simd::int8 indices[Indices] = 
        {
            0, 2, 1,
            0, 3, 2
        };

        const size_t positionsDataSize = NumVertices * sizeof( simd::float3 );
        const size_t colorDataSize = NumVertices * sizeof( simd::float3 );
        const size_t indicesDataSize = Indices * sizeof(simd::int8);

        _pVertexPositionsBuffer = _pDevice->newBuffer( positionsDataSize, MTL::ResourceStorageModeManaged );
        _pVertexColorsBuffer = _pDevice->newBuffer( colorDataSize, MTL::ResourceStorageModeManaged );
        _pIndexBuffer = _pDevice->newBuffer(indicesDataSize, MTL::ResourceStorageModeManaged);

        memcpy( _pVertexPositionsBuffer->contents(), positions, positionsDataSize );
        memcpy( _pVertexColorsBuffer->contents(), colors, colorDataSize );
        memcpy( _pIndexBuffer->contents(), indices, indicesDataSize);

        _pVertexPositionsBuffer->didModifyRange( NS::Range::Make( 0, _pVertexPositionsBuffer->length() ) );
        _pVertexColorsBuffer->didModifyRange( NS::Range::Make( 0, _pVertexColorsBuffer->length() ) );
        _pIndexBuffer->didModifyRange( NS::Range::Make( 0, _pIndexBuffer->length() ) );
    }

}
