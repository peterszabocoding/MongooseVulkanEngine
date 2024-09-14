#include "macosWindow.h"

#include <iostream>
#include <stdio.h>

namespace Raytracing {

    MacOSWindow::MacOSWindow(const WindowParams params): Window(params)
    {
        // Setup application window
        CGRect frame = (CGRect){ {0.0, 0.0}, {static_cast<CGFloat>(params.width), static_cast<CGFloat>(params.height)} };

        _pWindow = NS::Window::alloc()->init(
            frame,
            NS::WindowStyleMaskClosable | NS::WindowStyleMaskTitled,
            NS::BackingStoreBuffered,
            false );
        _pWindow->setTitle( NS::String::string( params.title, NS::StringEncoding::UTF8StringEncoding ) );


        // Moves the window to the front of the screen list, within its level, and makes it the key window; 
        // that is, it shows the window.
        _pWindow->makeKeyAndOrderFront( nullptr );

        _pDevice = MTL::CreateSystemDefaultDevice();
        _pViewDelegate = new MTKViewDelegate(*this);

        _pMtkView = MTK::View::alloc()->init( frame, _pDevice );
        _pMtkView->setColorPixelFormat( MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB );
        _pMtkView->setClearColor( MTL::ClearColor::Make( 0.0, 0.0, 0.0, 1.0 ) );
        _pMtkView->setDelegate( _pViewDelegate );

        _pWindow->setContentView( _pMtkView );

        camera.SetResolution(params.width, params.height);
        renderer.SetMTLDevice(_pDevice);
    }

    MacOSWindow::~MacOSWindow()
    {
        _pMtkView->release();
        _pWindow->release();
        _pDevice->release();

        delete _pViewDelegate;
    }

    void MacOSWindow::OnUpdate()
    {
        renderer.Render(camera);
        renderer.Draw(_pViewDelegate->getView());
    }

}
