
#include <Windows.h>

#include "basicTypes.h"
#include "engine.h"
#include "config.h"

int32 WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int32 nCmdShow)
{
	int32 returnCode = 0;
	{
		Hydrogen::Engine engine{};

		returnCode = engine.Run(lpCmdLine);
	}

	return returnCode;
}
