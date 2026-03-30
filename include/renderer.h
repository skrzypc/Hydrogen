#pragma once

#include "device.h"
#include "swapChain.h"
#include "frameGraph.h"

namespace Hydrogen
{
	class Renderer
	{
	public:
		Renderer() = default;
		~Renderer() = default;
		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = default;
		Renderer& operator=(Renderer&&) noexcept = default;

		void Initialize(HWND hWnd);
		void RenderFrame();

	private:
		GpuDevice m_gpuDevice;
		SwapChain m_swapChain;

		FrameGraph m_frameGraph;
		//GpuScene m_gpuScene;
	};
}