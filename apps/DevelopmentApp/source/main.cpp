
#include <Windows.h>

#include "app.h"

int32 WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int32 nCmdShow)
{
    // TODO: Handle command line arguments

    Application::Config appConfig{};
    appConfig.windowWidth = 3200;
    appConfig.windowHeight = 1800;
    appConfig.windowTitle = L"Development Application";

    Application app(appConfig);
    return app.Run();
}
