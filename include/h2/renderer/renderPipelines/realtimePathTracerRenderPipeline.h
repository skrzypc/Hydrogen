#pragma once

#include <h2/renderer/renderPipelines/renderPipeline.h>
#include <h2/renderer/passes/gBufferPass.h>
#include <h2/renderer/passes/buildTlasPass.h>
#include <h2/renderer/passes/tonemapPass.h>
#include <h2/renderer/passes/restirInitialCandidateGenerationPass.h>
#include <h2/renderer/passes/restirShadingPass.h>
#include <h2/renderer/passes/debug/accumulationPass.h>

namespace Hydrogen
{
    class RealtimePathTracerRenderPipeline : public IRenderPipeline
    {
    public:
        static constexpr std::string_view Name = "RealtimePathTracer";

        void Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler, uint32 renderWidth,
                        uint32 renderHeight) override;
        void Shutdown() override;

        std::string_view Render(FrameGraph& frameGraph, const FrameContext& frameContext) override;
        void BuildUI() override;

        std::string_view GetName() const override
        {
            return Name;
        }

    private:
        void DefineFrameGraphResources(FrameGraph& frameGraph, const FrameContext& frameContext);

    private:
        bool m_accumulate = false;

        GBufferPass m_gBufferPass{};
        ReSTIRInitialCandidateGenerationPass m_restirInitialCandidateGenerationPass{};
        ReSTIRShadingPass m_restirShadingPass{};
        BuildTlasPass m_buildTlasPass{};
        TonemapPass m_tonemapPass{};
        AccumulationPass m_accumulationPass{};
    };
} // namespace Hydrogen
