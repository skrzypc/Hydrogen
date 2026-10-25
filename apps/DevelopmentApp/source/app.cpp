
#include <Windows.h>

#include <h2/core/basicTypes.h>

#include "app.h"
#include <h2/engine.h>

extern "C"
{
    __declspec(dllexport) extern const UINT D3D12SDKVersion = 619;
}
extern "C"
{
    __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\";
}

int32 WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int32 nCmdShow)
{
    int32 returnCode = 0;
    {
        Hydrogen::Engine engine{};

        returnCode = engine.Run(lpCmdLine);
    }

    return returnCode;
}
