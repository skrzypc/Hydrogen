#pragma once

#include "device.h"
#include "swapChain.h"
#include "frameGraph.h"
#include "shaderCompiler.h"
#include "uploadRingBuffer.h"
#include "gpuUploader.h"
#include "renderPasses/clearPass.h"
#include "renderPasses/animateBackground.h"
#include "renderPasses/copyPass.h"
#include "renderPasses/overlappingRectsPass.h"

namespace Hydrogen
{
	class Renderer
	{
	public:
		Renderer() = default;
		~Renderer();
		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = default;
		Renderer& operator=(Renderer&&) noexcept = default;

		void Initialize(HWND hWnd);
		void RenderFrame();

	private:
		void BeginFrame(uint32 frameIndex);
		void EndFrame(uint32 frameIndex, uint64 fenceValue);

		GpuDevice m_gpuDevice;
		SwapChain m_swapChain;

		FrameGraph m_frameGraph;
		ShaderCompiler m_shaderCompiler;
		UploadRingBuffer m_uploadBuffer{};
		GpuUploader m_gpuUploader{};

		uint64 m_frameFenceValues[Config::FramesInFlight] = {};

		ClearPass m_clearPass{};
		AnimateBackgroundPass m_animateBackgroundPass{};
		CopyPass m_copyPass{};
		OverlappingRectsPass m_overlappingRectsPass{};
		//GpuScene m_gpuScene;
	};
}