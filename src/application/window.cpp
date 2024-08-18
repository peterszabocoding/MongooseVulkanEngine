#include "window.h"
#include <GLFW/glfw3.h>

Window::Window(WindowParams params, OnWindowCloseCallback callback)
{
    windowCloseCallback = callback;
    if (!glfwInit()) return;

    glfwWindow = glfwCreateWindow(params.width, params.height, params.title, NULL, NULL);
    if (!glfwWindow)
    {
        Terminate();
        return;
    }

    glfwMakeContextCurrent(glfwWindow);
}

void Window::OnUpdate()
{
    if (glfwWindowShouldClose(glfwWindow)) windowCloseCallback();
    
    glClear(GL_COLOR_BUFFER_BIT);
    glfwSwapBuffers(glfwWindow);
    glfwPollEvents();
}

void Window::Terminate()
{
    glfwTerminate();
}
