#include "application/application.h"

int main(int argc, char* argv[])
{
	Raytracing::Application* application = Raytracing::Application::Create();
	application->OnCreate();
	application->Run();

	return 0;
}
