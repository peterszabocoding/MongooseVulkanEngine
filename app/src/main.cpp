#include <application/demo_application.h>
#include "application/application.h"
#include "util/log.h"

int main(int argc, char* argv[])
{
    MongooseVK::Log::Init();

    const MongooseVK::ApplicationConfig config = {
        .appName = "MongooseVKDemo",
        .windowTitle = "Mongoose Vulkan Demo",
        .versionName = "0.1.0",
        .appVersion = Version{0, 1, 0},
        .windowWidth = 1675,
        .windowHeight = 935
    };

    MongooseVK::Application* application = new VulkanDemo::DemoApplication(config);

    application->Init();
    application->Run();

    return 0;
}
