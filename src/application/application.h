#pragma once

static constexpr char* WINDOW_TITLE = "Raytracing in one weekend";
static constexpr int WINDOW_WIDTH = 800;
static constexpr int WINDOW_HEIGHT = 800;

namespace Raytracing {
    class Application {
        public:
            virtual ~Application() {}

            virtual void OnCreate() {}
            virtual void Run() {}

            static Application* Create();
    };
}
