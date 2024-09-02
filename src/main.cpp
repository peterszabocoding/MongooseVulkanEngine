
#include "application/application.h"
#include <iostream>

int main( int argc, char* argv[] )
{
    Raytracing::Application* application = Raytracing::Application::Create();
    application->Run();

    return 0;
}
