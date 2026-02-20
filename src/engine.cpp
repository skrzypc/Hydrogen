
#include "engine.h"
#include "config.h"
#include "logger.h"

namespace Hydrogen
{
	int32 Engine::Run(LPSTR commandLineArgs)
	{
		Logger::Initialize();

		if (Hydrogen::Config::WaitForDebugger)
		{
			while (!::IsDebuggerPresent())
			{
				::Sleep(1000);

				H2_INFO(eLogLocation::Engine, eLogLevel::Verbose, "Waiting for debugger.");
			}
		}

		m_window.Create(Config::WindowWidth, Config::WindowHeight, L"Hydrogen Engine");

		int32 returnCode = 0;
		while (true)
		{
			if (const auto ecode = m_window.ProcessMessages())
			{
				// If return optional has value, it means that
				// we're quitting so return exit code.
				returnCode = *ecode;

				break;
			}
		}

		return returnCode;
	}
}
