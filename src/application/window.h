#pragma once

#include <functional>

class GLFWwindow;

namespace Raytracing {

    typedef std::function<void()> OnWindowCloseCallback;

    struct WindowParams {
        unsigned int width = 800;
        unsigned int height = 800;
        const char* title = "Default Title";
    };

    class Window {

        public:
            Window(const WindowParams params) 
            {
                windowParams = params;
            }
            virtual ~Window() = default;

            virtual void OnCreate() {}
            virtual void OnUpdate() {}

            virtual void SetOnWindowCloseCallback(OnWindowCloseCallback callback)
            {
                windowCloseCallback = callback;
            } 

            static Window* Create(const WindowParams params = WindowParams());

        protected:
            WindowParams windowParams;
            OnWindowCloseCallback windowCloseCallback;

    };

}
