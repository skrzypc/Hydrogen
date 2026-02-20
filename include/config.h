#pragma once

#include "basicTypes.h"

#include "logger.h"

namespace Hydrogen
{
	struct Config
	{
		static constexpr bool WaitForDebugger = false;

		static constexpr uint32 MaxFramesInFlight = 3;

		static constexpr uint32 WindowWidth = 1920;
		static constexpr uint32 WindowHeight = 1080;

		static constexpr eLogLevel LogLevel = eLogLevel::Verbose;
	};
}