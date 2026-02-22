#pragma once

#include "basicTypes.h"

#include "logger.h"
#include "device.h"

namespace Hydrogen
{
	struct Config
	{
		static constexpr bool WaitForDebugger = false;

		static constexpr uint32 FramesInFlight = 3;

		static constexpr uint32 WindowWidth = 1920;
		static constexpr uint32 WindowHeight = 1080;

		static constexpr eLogLevel LogLevel = eLogLevel::Verbose;

		static constexpr bool EnableGPUBasedValidation = false;
		static constexpr eAdapterVendor RequestedAdapter = eAdapterVendor::NVIDIA;
	};
}