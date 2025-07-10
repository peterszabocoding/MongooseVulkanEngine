#include "demo_application.h"
#include "demo_window.h"

namespace VulkanDemo
{
    MongooseVK::Window* DemoApplication::CreateWindow(MongooseVK::WindowParams params)
    {
        return new DemoWindow(params);
    }
}
