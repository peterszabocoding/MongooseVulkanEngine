#include "macosApplication.h"
#include "renderer/metalRenderer.h"
#include "macosWindow.h"

#include <iostream>

namespace Raytracing {

    MacOSApplication::MacOSApplication()
    {
        pAutoreleasePool = NS::AutoreleasePool::alloc()->init();
        pSharedApplication = NS::Application::sharedApplication();

        del = new AppDelegate(*this);
        pSharedApplication->setDelegate( del );
    }

    MacOSApplication::~MacOSApplication()
    {
        pAutoreleasePool->release();
        delete del;
    }

    void MacOSApplication::OnCreate()
    {
        // Setup application menu bar
        NS::Menu* pMenu = CreateMenuBar();
        NS::Application* pApp = reinterpret_cast<NS::Application*>( del->getLatestNotification()->object() );

        pApp->setMainMenu( pMenu );
        pApp->activateIgnoringOtherApps( true );
        pApp->setActivationPolicy( NS::ActivationPolicy::ActivationPolicyRegular );

        WindowParams params;
        params.width = 600;
        params.height = 600;
        params.title = "Raytracing in one weekend";
        window = new MacOSWindow(params);
    }

    void MacOSApplication::Run()
    {
        pSharedApplication->run();
    }

    NS::Menu* MacOSApplication::CreateMenuBar()
    {
        using NS::StringEncoding::UTF8StringEncoding;

        NS::Menu* pMainMenu = NS::Menu::alloc()->init();
        NS::MenuItem* pAppMenuItem = NS::MenuItem::alloc()->init();
        NS::Menu* pAppMenu = NS::Menu::alloc()->init( NS::String::string( "Appname", UTF8StringEncoding ) );

        NS::String* appName = NS::RunningApplication::currentApplication()->localizedName();
        NS::String* quitItemName = NS::String::string( "Quit ", UTF8StringEncoding )->stringByAppendingString( appName );
        SEL quitCb = NS::MenuItem::registerActionCallback( "appQuit", [](void*,SEL,const NS::Object* pSender){
            auto pApp = NS::Application::sharedApplication();
            pApp->terminate( pSender );
        } );

        NS::MenuItem* pAppQuitItem = pAppMenu->addItem( quitItemName, quitCb, NS::String::string( "q", UTF8StringEncoding ) );
        pAppQuitItem->setKeyEquivalentModifierMask( NS::EventModifierFlagCommand );
        pAppMenuItem->setSubmenu( pAppMenu );

        NS::MenuItem* pWindowMenuItem = NS::MenuItem::alloc()->init();
        NS::Menu* pWindowMenu = NS::Menu::alloc()->init( NS::String::string( "Window", UTF8StringEncoding ) );

        SEL closeWindowCb = NS::MenuItem::registerActionCallback( "windowClose", [](void*, SEL, const NS::Object*){
            auto pApp = NS::Application::sharedApplication();
                pApp->windows()->object< NS::Window >(0)->close();
        } );
        NS::MenuItem* pCloseWindowItem = pWindowMenu->addItem( NS::String::string( "Close Window", UTF8StringEncoding ), closeWindowCb, NS::String::string( "w", UTF8StringEncoding ) );
        pCloseWindowItem->setKeyEquivalentModifierMask( NS::EventModifierFlagCommand );

        pWindowMenuItem->setSubmenu( pWindowMenu );

        pMainMenu->addItem( pAppMenuItem );
        pMainMenu->addItem( pWindowMenuItem );

        pAppMenuItem->release();
        pWindowMenuItem->release();
        pAppMenu->release();
        pWindowMenu->release();

        return pMainMenu->autorelease();
    }

}
