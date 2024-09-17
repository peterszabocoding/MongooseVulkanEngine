#include "application/application.h"
#include <iostream>

int main(int argc, char* argv[])
{
	Raytracing::Application* application = Raytracing::Application::Create();
	application->OnCreate();
	application->Run();

	return 0;
}
