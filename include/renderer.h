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
#include <memory>

#include "renderBackends/renderBackend.h"
#include "renderPasses/copyPass.h"
#include "renderPasses/imguiPass.h"

struct ImDrawData;

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
		void RenderFrame(const RenderScene& renderScene, ImDrawData* drawData);

		void SetUploadQueue(AssetUploadQueue* pQueue) { m_pUploadQueue = pQueue; }
		void SwitchBackend(eRenderBackendType type);
		void BuildBackendUI();

	private:
		void BeginFrame();
		void EndFrame(uint32 frameIndex, uint64 fenceValue);

		void UpdateFrameData(const RenderScene& renderScene);
		void ProcessUploadQueue();
		void CreateBackend(eRenderBackendType type);

		GpuDevice m_gpuDevice;
		SwapChain m_swapChain;

		FrameGraph m_frameGraph;

		ShaderCompiler m_shaderCompiler;

		UploadRingBuffer m_uploadBuffer{};
		GpuUploader m_gpuUploader{};
		AssetUploadQueue* m_pUploadQueue = nullptr;

		GpuScene m_gpuScene{};

		std::array<uint64, Config::FramesInFlight> m_frameFenceValues{};
		uint32 m_currentFrameIndex = 0;
		float32 m_time = 0.0f;

		std::unique_ptr<IRenderBackend> m_backend;
		eRenderBackendType m_backendType = eRenderBackendType::Raster;

		CopyPass m_copyPass{};
		ImguiPass m_imguiPass{};
	};
}