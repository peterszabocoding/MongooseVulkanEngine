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
    }

    void MetalRenderer::draw(MTK::View *pView)
    {
        NS::AutoreleasePool* pPool = NS::AutoreleasePool::alloc()->init();

        MTL::CommandBuffer* pCmd = _pCommandQueue->commandBuffer();
        MTL::RenderPassDescriptor* pRpd = pView->currentRenderPassDescriptor();
        MTL::RenderCommandEncoder* pEnc = pCmd->renderCommandEncoder( pRpd );
        pEnc->endEncoding();
        pCmd->presentDrawable( pView->currentDrawable() );
        pCmd->commit();

        pPool->release();
    }

}
