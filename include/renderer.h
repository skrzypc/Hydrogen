#pragma once

#include <optional>
#include <vector>

#include "device.h"
#include "swapChain.h"
#include "frameGraph.h"
#include "shaderCompiler.h"
#include "uploadRingBuffer.h"
#include "gpuUploader.h"
#include "gpuScene.h"
#include "gpuMesh.h"
#include "renderPasses/clearPass.h"
#include "renderPasses/copyPass.h"
#include "renderPasses/meshPass.h"

namespace Hydrogen
{
	class AssetUploadQueue;
	struct RenderScene;

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
		void RenderFrame(const RenderScene& renderScene);

		void SetUploadQueue(AssetUploadQueue* pQueue) { m_pUploadQueue = pQueue; }

	private:
		void BeginFrame(uint32 frameIndex);
		void EndFrame(uint32 frameIndex, uint64 fenceValue);

		void UpdateFrameData(const RenderScene& renderScene);
		void ProcessUploadQueue();

		GpuDevice m_gpuDevice;
		SwapChain m_swapChain;

		FrameGraph m_frameGraph;

		ShaderCompiler m_shaderCompiler;

		UploadRingBuffer m_uploadBuffer{};
		GpuUploader m_gpuUploader{};
		AssetUploadQueue* m_pUploadQueue = nullptr;

		GpuScene m_gpuScene{};

		std::array<uint64, Config::FramesInFlight> m_frameFenceValues{};
		float32 m_time = 0.0f;

		ClearPass m_clearPass{};
		CopyPass m_copyPass{};
		MeshPass m_meshPass{};
	};
}