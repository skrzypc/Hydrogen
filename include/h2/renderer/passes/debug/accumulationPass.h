#pragma once

#include <memory>
#include <optional>
#include <string>

#include <h2/renderer/passes/renderPass.h>
#include <h2/renderer/framegraph/frameGraphBuilder.h>
#include <h2/renderer/gpu/pipeline/pipelineState.h>
#include <h2/renderer/gpu/device.h>

namespace Hydrogen
{
    // Debug pass: replaces `sceneColor` with its running average over frames, so a noisy estimator can be compared
    // with a converged reference. Insert it anywhere after the pass that writes `sceneColor`.
    class AccumulationPass : public IRenderPass
    {
    public:
        struct PushConstants
        {
            uint32 sceneColorIndex = 0;
            uint32 accumulationTargetIndex = 0;
            uint32 accumulatedFramesCount = 0;
        };

        static constexpr DXGI_FORMAT AccumulationFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;

        std::string sceneColor = "SceneColor";
        std::string accumulationTarget = "AccumulationTarget";

        // Set before AddPass each frame. Accumulation also restarts when the pass skipped a frame.
        uint64 frameNumber = 0;
        bool resetAccumulation = false;

        void Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler) override;
        void CreateAccumulationTarget(uint32 width, uint32 height);
        void Setup(FGBuilder& builder) override;
        void Execute(FGExecuteContext& fgExecuteContext, GraphicsContext& graphicsContext) override;

        Texture* GetAccumulationTarget() const
        {
            return m_pAccumulationTarget.get();
        }
        uint32 GetAccumulatedFramesCount() const
        {
            return m_accumulatedFramesCount;
        }

    private:
        static constexpr uint32 ThreadGroupSize = 8;

        GpuDevice* m_pDevice = nullptr;
        PipelineState m_pso{};

        std::unique_ptr<Texture> m_pAccumulationTarget{};
        uint32 m_accumulatedFramesCount = 0;
        std::optional<uint64> m_lastFrameNumber{};

        uint32 m_width = 0;
        uint32 m_height = 0;
    };
} // namespace Hydrogen
