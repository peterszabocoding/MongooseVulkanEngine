#include "application.h"
#include "renderer/renderer.h"
#include <iostream>

Application::Application()
{
    WindowParams params;
    params.title = WINDOW_TITLE;
    params.width = WINDOW_WIDTH;
    params.height = WINDOW_HEIGHT;

    window = new Window(params, [&]() {
        running = false;
        window->Terminate();
    });
    OnCreate();
}

Application::~Application()
{
    OnDestroy();
    delete window;

}

void Application::OnCreate()
{
    running = true;
    Renderer::Render(WINDOW_WIDTH, WINDOW_HEIGHT);
}

void Application::OnUpdate()
{
    window->OnUpdate();
}

void Application::OnDestroy()
{
}

bool Application::isRunning()
{
    return running;
}
