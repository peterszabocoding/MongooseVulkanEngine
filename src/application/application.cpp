#include "application.h"
#include "renderer/renderer.h"
#include <iostream>

Application::Application()
{
    window = new Window(WindowParams(), [&]() {
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
    Renderer::Render(800, 800);
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
