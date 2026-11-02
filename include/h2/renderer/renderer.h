#pragma once

#include <optional>
#include <vector>

#include <h2/hydrogenConfig.h>
#include <h2/scene/scene.h>

#include <h2/renderer/gpu/device.h>
#include <h2/renderer/gpu/swapChain.h>
#include <h2/renderer/framegraph/frameGraph.h>
#include <h2/shaders/shaderCompiler.h>
#include <h2/renderer/gpu/resources/uploadRingBuffer.h>
#include <h2/renderer/gpu/resources/uploadBuffer.h>
#include <h2/assets/gpuUploader.h>
#include <h2/renderer/gpu/scene/gpuScene.h>
#include <h2/renderer/gpu/resources/gpuMesh.h>
#include <h2/renderer/frameContext.h>
#include <memory>

#include <h2/renderer/renderPipelines/renderPipeline.h>
#include <h2/renderer/passes/copyPass.h>
#include <h2/renderer/passes/imguiPass.h>
#include <h2/renderer/rendererUiRequest.h>

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

        void Initialize(uint32 renderWidth, uint32 renderHeight, HWND hWnd, const RuntimePaths& paths);
        void SetUploadQueue(AssetUploadQueue* pQueue)
        {
            m_pUploadQueue = pQueue;
        }

        void SubmitFrame(const Scene& scene, ImDrawData* drawData, const RendererUiRequest& uiRequest, float64 time,
                         float32 deltaTime);
        // Main thread only. Reads renderer settings and stats, never touches GPU objects or frame graph state.
        // Changes the user asks for are returned and applied by the next SubmitFrame.
        [[nodiscard]] RendererUiRequest BuildUi();

    private:
        void RenderFrame(const RenderScene& renderScene, ImDrawData* drawData, float64 time, float32 deltaTime);

        void SwitchRenderPipeline(eRenderPipelineType type);

        [[nodiscard]] FrameContext BeginFrame(const RenderScene& renderScene, float64 time, float32 deltaTime);
        void EndFrame(uint32 frameIndex, uint64 fenceValue);

        void UpdateFrameData(const FrameContext& frameContext);
        void ProcessUploadQueue();
        void CreateRenderPipeline(eRenderPipelineType type);

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

        std::array<uint64, StaticConfig::FramesInFlight> m_frameFenceValues{};

        std::unique_ptr<IRenderPipeline> m_renderPipeline = nullptr;
        eRenderPipelineType m_renderPipelineType = eRenderPipelineType::RayTracing;

        CopyPass m_copyPass{};
        ImguiPass m_imguiPass{};

        CameraData m_previousCameraData{};

        uint32 m_renderWidth = 0u;
        uint32 m_renderHeight = 0u;
    };
} // namespace Hydrogen
