#pragma once

#include "application/application.h"
#include "renderer/metal/metalRenderer.h"

#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define MTK_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#include <Metal/Metal.hpp>
#include <AppKit/AppKit.hpp>
#include <MetalKit/MetalKit.hpp>

namespace Raytracing {

    class MacOSWindow;

    class AppDelegate : public NS::ApplicationDelegate
    {
        public:
            AppDelegate(Application& app): application(app) {}
            virtual ~AppDelegate() {};

            virtual void applicationWillFinishLaunching( NS::Notification* pNotification ) override {}
            virtual void applicationDidFinishLaunching( NS::Notification* pNotification ) override
            {
                notification = pNotification;
                application.OnCreate();
            }
            virtual bool applicationShouldTerminateAfterLastWindowClosed( NS::Application* pSender ) override
            {
                return true;
            }

            NS::Notification* getLatestNotification() 
            {
                return notification;
            }

        private:
            Application& application;
            NS::Notification* notification;
    };

    class MacOSApplication: public Application {

        public:
            MacOSApplication();
            virtual ~MacOSApplication();

            virtual void OnCreate() override;
            virtual void Run() override;

            void CreateWindow();

            NS::Menu* CreateMenuBar();

        private:
            AppDelegate* del;

            MacOSWindow* window;
            NS::Application* pSharedApplication;
            NS::AutoreleasePool* pAutoreleasePool;
    };
}
