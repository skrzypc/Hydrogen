#pragma once

#include "renderBackend.h"
#include "renderPasses/gBufferPass.h"
#include "renderPasses/lightingPass.h"
#include "renderPasses/tonemapPass.h"

namespace Hydrogen
{
    class DeferredBackend : public IRenderBackend
    {
    public:
        void Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler) override;
        void Shutdown() override;

        std::string_view Render(FrameGraph& frameGraph, const FrameContext& frameContext) override;
        void BuildUI() override;

        const char* GetName() const override { return "Deferred"; }

    private:
        void DefineFrameGraphResources(FrameGraph& frameGraph, const FrameContext& frameContext);

    private:
        GBufferPass m_gBufferPass{};
        LightingPass m_lightingPass{};
        TonemapPass m_tonemapPass{};
    };
}
