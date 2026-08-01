#pragma once

#include <optional>
#include <vector>

#include "device.h"
#include "swapChain.h"
#include "frameGraph.h"
#include "shaderCompiler.h"
#include "uploadRingBuffer.h"
#include "uploadBuffer.h"
#include "gpuUploader.h"
#include "gpuScene.h"
#include "gpuMesh.h"
#include "frameContext.h"
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
		void RenderFrame(const RenderScene& renderScene, ImDrawData* drawData, float64 time, float32 deltaTime);

		void SetUploadQueue(AssetUploadQueue* pQueue) { m_pUploadQueue = pQueue; }
		void SwitchBackend(eRenderBackendType type);
		void BuildBackendUI();

	private:
		[[nodiscard]] FrameContext BeginFrame(const RenderScene& renderScene, float64 time, float32 deltaTime);
		void EndFrame(uint32 frameIndex, uint64 fenceValue);

		void UpdateFrameData(const FrameContext& frameContext);
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

		std::unique_ptr<UploadBuffer> m_viewBuffer = nullptr;
		ShaderResourceViewHandle m_viewBufferSrv{};
		uint32 m_maxViews = 16;

		std::array<uint64, Config::FramesInFlight> m_frameFenceValues{};

		std::unique_ptr<IRenderBackend> m_backend = nullptr;
		eRenderBackendType m_backendType = eRenderBackendType::RayTracing;

		CopyPass m_copyPass{};
		ImguiPass m_imguiPass{};
	};
}