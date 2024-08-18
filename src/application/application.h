#include "window.h"

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