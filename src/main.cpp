#include "application/application.h"
#include "util/log.h"

int main(int argc, char* argv[])
{
	Raytracing::Log::Init();

	Raytracing::Application* application = Raytracing::Application::Create();
	application->OnCreate();
	application->Run();

	return 0;
}
