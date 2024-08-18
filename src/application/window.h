#include <functional>

class GLFWwindow;

typedef std::function<void()> OnWindowCloseCallback;

struct WindowParams {
    unsigned int width = 800;
    unsigned int height = 800;
    const char* title = "Default Title";
};

class Window {

    public:

        Window(WindowParams params = WindowParams(), OnWindowCloseCallback callback = []{});
        ~Window() = default;

        void OnUpdate();
        void Terminate();

    private:
        GLFWwindow* glfwWindow;
        OnWindowCloseCallback windowCloseCallback;

};
