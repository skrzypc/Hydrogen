
#include <Windows.h>

#include "app.h"

extern "C"
{
    __declspec(dllexport) extern const UINT D3D12SDKVersion = 619;
    __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\";
}

int32 WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int32 nCmdShow)
{
    // TODO: Handle command line arguments

    Application::Config appConfig{};
    appConfig.windowWidth = 2560;
    appConfig.windowHeight = 1440;
    appConfig.windowTitle = L"Development Application";

    Application app(appConfig);
    return app.Run();
}
