
#include "application/application.h"

int main() {

    Application* application = new Application();

    while(application->isRunning())
    {
        application->OnUpdate();
    }

    delete application;
}