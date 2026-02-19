#pragma once

#include "basicTypes.h"
#include "window.h"

namespace Hydrogen
{
	class Engine
	{
	public:
		Engine() = default;
		~Engine() = default;
		Engine(const Engine&) = delete;
		const Engine& operator=(const Engine&) = delete;
		Engine(Engine&&) noexcept = default;
		Engine& operator=(Engine&&) noexcept = default;

		// TODO: Parse command line arguments before passing them to the engine.
		int32 Run(LPSTR commandLineArgs);

	private:
		Window m_window{};
	};
}