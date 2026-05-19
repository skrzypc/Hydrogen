#pragma once

#include "basicTypes.h"
#include "window.h"
#include "renderer.h"
#include "scene.h"
#include "assetRegistry.h"
#include "entity.h"
#include "timer.h"

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
		Renderer m_renderer{};
		Scene m_scene{};
		AssetRegistry m_assetRegistry{};

		Entity m_activeCamera{};
		float32 m_yaw = 0.0f;
		float32 m_pitch = 0.0f;
		float32 m_cameraSpeed = 2.0f;
		Timer m_frameTimer{};

	};
}