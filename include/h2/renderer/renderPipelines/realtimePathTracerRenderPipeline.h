#pragma once

#include <h2/renderer/renderPipelines/renderPipeline.h>
#include <h2/renderer/passes/gBufferPass.h>
#include <h2/renderer/passes/buildTlasPass.h>
#include <h2/renderer/passes/tonemapPass.h>
#include <h2/renderer/passes/dummyPass.h>
#include <h2/renderer/passes/restirInitialCandidateGenerationPass.h>
#include <h2/renderer/passes/restirShadingPass.h>

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
        // Values match the indices hard-coded in dummyShader.cs.hlsl.
        enum class eGBufferView : uint32
        {
            Albedo = 0,
            Normal,
            Roughness,
            Metallic,
            Depth,
            Count
        };

        void DefineFrameGraphResources(FrameGraph& frameGraph, const FrameContext& frameContext);

    private:
        bool m_showGBuffer = false;
        eGBufferView m_gBufferView = eGBufferView::Albedo;

        GBufferPass m_gBufferPass{};
        ReSTIRInitialCandidateGenerationPass m_restirInitialCandidateGenerationPass{};
        ReSTIRShadingPass m_restirShadingPass{};
        DummyPass m_dummyPass{};
        BuildTlasPass m_buildTlasPass{};
        TonemapPass m_tonemapPass{};
    };
} // namespace Hydrogen
