#pragma once

#include "application/window.h"
#include "renderer/metal/metalRenderer.h"

#include <Metal/Metal.hpp>
#include <AppKit/AppKit.hpp>
#include <MetalKit/MetalKit.hpp>

namespace Raytracing {

    class MTKViewDelegate : public MTK::ViewDelegate
    {
        public:
            MTKViewDelegate(Window& windowRef): window(windowRef) {}

            virtual void drawInMTKView( MTK::View* pView ) override
            {
                view = pView;
                window.OnUpdate();
            }

            MTK::View* getView() const { return view; }

        private:
            Window& window;
            MTK::View* view;
    };

    class MacOSWindow: public Window {

        public:
            MacOSWindow(const WindowParams params);
            ~MacOSWindow();

            virtual void OnUpdate() override;

        private:
            NS::Window* _pWindow;
            MTK::View* _pMtkView;
            MTL::Device* _pDevice;
            MTKViewDelegate* _pViewDelegate = nullptr;

            MetalRenderer renderer;
            Camera camera;
    };

}