#pragma once

#include "device.h"
#include "swapChain.h"
#include "frameGraph.h"
#include "shaderCompiler.h"
#include "renderPasses/clearPass.h"
#include "renderPasses/testTrianglePass.h"

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
		ShaderCompiler m_shaderCompiler;

		ClearPass m_clearPass{};
		TestTrianglePass m_testTrianglePass{};
		//GpuScene m_gpuScene;
	};
}