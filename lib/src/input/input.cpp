#include "input/input.h"

#include <GLFW/glfw3.h>
#include "application/application.h"

namespace Raytracing {

    Input* Input::s_Instance = new Input();

    bool Input::IsKeyPressedImpl(int keycode)
    {
        GLFWwindow* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
        auto state = glfwGetKey(window, keycode);

        return state == GLFW_PRESS || state == GLFW_REPEAT;
    }

    bool Input::IsMousePressedImpl(int button)
    {
        GLFWwindow* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
        auto state = glfwGetMouseButton(window, button);

        return state == GLFW_PRESS;
    }

    std::pair<float, float> Input::GetMousePosImpl()
    {
        GLFWwindow* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
        double x, y;
        glfwGetCursorPos(window, &x, &y);

        return std::pair((float)x, (float)y);
    }

    float Input::GetMousePosXImpl()
    {
        auto [x, y] = GetMousePosImpl();
        return x;
    }

    float Input::GetMousePosYImpl()
    {
        auto [x, y] = GetMousePosImpl();
        return y;
    }

}


