#pragma once

#include <h2/renderer/renderPipelines/renderPipeline.h>
#include <h2/renderer/passes/gBufferPass.h>
#include <h2/renderer/passes/buildTlasPass.h>
#include <h2/renderer/passes/tonemapPass.h>

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
        GBufferPass m_gBufferPass{};
        BuildTlasPass m_buildTlasPass{};
        TonemapPass m_tonemapPass{};
    };
} // namespace Hydrogen
