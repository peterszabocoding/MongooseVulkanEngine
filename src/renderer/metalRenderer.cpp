#include "metalRenderer.h"
#include <iostream>

#include "math/color.h"

#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define MTK_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#include <Metal/Metal.hpp>
#include <AppKit/AppKit.hpp>
#include <MetalKit/MetalKit.hpp>

#include <ctime> 

#include "util/timer.h"

namespace Raytracing {

    MetalRenderer::MetalRenderer()
    {
    }

    MetalRenderer::~MetalRenderer()
    {
        _pCommandQueue->release();
        _pDevice->release();
        if(image) delete[] image;
    }

    void MetalRenderer::SetResolution(unsigned long image_width, unsigned long image_height)
    {
        renderWidth = image_width;
        renderHeight = image_height;

        image = new simd::uint1[renderWidth * renderHeight];
    }

    void MetalRenderer::Render()
    {
        Timer timer;
        if (renderWidth == 0 || renderHeight == 0) return;

        // Render
        for (int j = 0; j < renderHeight; j++) {
            for (int i = 0; i < renderWidth; i++) {
                vec3 pixelColor(
                    double(i) / (renderWidth-1), 
                    double(j) / (renderHeight-1), 
                    0.0);

                unsigned int pixel = i + j * renderWidth;
                
                // Translate the [0,1] component values to the byte range [0,255].
                uint8_t rbyte = uint8_t(255.999 * pixelColor.x());
                uint8_t gbyte = uint8_t(255.999 * pixelColor.y());
                uint8_t bbyte = uint8_t(255.999 * pixelColor.z());

                image[pixel] = (255 << 24) | (rbyte << 16) | (gbyte << 8) | (bbyte);
            }
        }

        MTL::Region region = {
            0, 0, 0,                         // MTLOrigin
            renderWidth, renderHeight, 1     // MTLSize
        };

        _pTexture->replaceRegion(region, 0, image, 4 * renderWidth);
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
        pEnc->setVertexBuffer( _pVertexBuffer, 0, 0 );

        pEnc->setFragmentTexture(_pTexture, 0);

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

        MetalVertex vertices[NumVertices] =
        {
            { { -1.0f, -1.0f, 0.0f }, { 0.0,  0.0f } },
            { { -1.0f, 1.0f,  0.0f }, { 0.0f, 1.0f } },
            { { 1.0f,  1.0f,  0.0f }, { 1.0f, 1.0f } },
            { { 1.0f,  -1.0f, 0.0f }, { 1.0f, 0.0f } }
        };

        simd::int8 indices[Indices] = 
        {
            0, 2, 1,
            0, 3, 2
        };

        const size_t positionsDataSize = NumVertices * sizeof( simd::float3 );
        const size_t colorDataSize = NumVertices * sizeof( simd::float3 );

        const size_t vertexDataSize = NumVertices * sizeof(MetalVertex);
        const size_t indicesDataSize = Indices * sizeof(simd::int8);

        _pVertexBuffer = _pDevice->newBuffer(vertexDataSize, MTL::ResourceStorageModeManaged);
        _pIndexBuffer = _pDevice->newBuffer(indicesDataSize, MTL::ResourceStorageModeManaged);

        memcpy( _pVertexBuffer->contents(), vertices, vertexDataSize);
        memcpy( _pIndexBuffer->contents(), indices, indicesDataSize);

        _pVertexBuffer->didModifyRange( NS::Range::Make( 0, _pVertexBuffer->length() ) );
        _pIndexBuffer->didModifyRange( NS::Range::Make( 0, _pIndexBuffer->length() ) );

        MTL::TextureDescriptor* textureDesc = MTL::TextureDescriptor::alloc()->init();
        textureDesc->setPixelFormat(MTL::PixelFormat::PixelFormatBGRA8Unorm);
        textureDesc->setWidth(400);
        textureDesc->setHeight(400);

        _pTexture = _pDevice->newTexture(textureDesc);
    }

}
