#include "application/application.h"
#include "util/log.h"
#include "util/thread_pool.h"

namespace MongooseVK
{
    Application* Application::s_Instance = nullptr;

    Application::Application(ApplicationConfig config)
    {
        ASSERT(!s_Instance, "Application has been initialized already");
        s_Instance = this;

        applicationInfo.appName = config.appName;
        applicationInfo.windowWidth = config.windowWidth;
        applicationInfo.windowHeight = config.windowHeight;
        applicationInfo.appVersion = config.appVersion;
        applicationInfo.versionName = config.appVersion.GetVersionName();
        applicationInfo.windowTitle = config.windowTitle;
    }

    Application::~Application()
    {
        delete window;
    }

    void Application::Init()
    {
        WindowParams params;
        params.title = applicationInfo.windowTitle.c_str();
        params.width = applicationInfo.windowWidth;
        params.height = applicationInfo.windowHeight;

        window = CreateWindow(params);
        window->SetOnWindowCloseCallback([&] { isRunning = false; });

        ThreadPool::Create();

        isRunning = true;
    }

    void Application::Run()
    {
        LOG_TRACE("Application Run");

        while (isRunning)
        {
            const float time = glfwGetTime();
            const float deltaTime = time - lastFrameTime;

            lastFrameTime = time;
            window->OnUpdate(deltaTime);
        }
    }

    Window* Application::CreateWindow(WindowParams params)
    {
        return new Window(params);
    }

    Application* Application::Create(const ApplicationConfig& config)
    {
        return new Application(config);
    }
}
