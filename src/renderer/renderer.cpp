#include "renderer.h"
#include "util/timer.h"

namespace Raytracing
{
    void Renderer::SetResolution(unsigned long image_width, unsigned long image_height)
    {
        renderWidth = image_width;
        renderHeight = image_height;
    }

    void Renderer::Render()
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

                unsigned int pixelCount = i + j * renderWidth;
                ProcessPixel(pixelCount, pixelColor);
            }
        }

        OnRenderFinished();
    }

}

