#include "window.h"

static constexpr char* WINDOW_TITLE = "Raytracing in one weekend";
static constexpr int WINDOW_WIDTH = 800;
static constexpr int WINDOW_HEIGHT = 800;

class Application {
    public:
        Application();
        ~Application();

        void OnCreate();
        void OnUpdate();
        void OnDestroy();

        bool isRunning();

    private:
        Window* window;
        bool running = false;
};