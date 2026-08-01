#pragma once

#include "renderBackend.h"
#include "renderPasses/clearPass.h"
#include "renderPasses/meshPass.h"
#include "renderPasses/buildTlasPass.h"

namespace Hydrogen
{
    class HybridBackend : public IRenderBackend
    {
    public:
        void Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler) override;
        void Shutdown() override;

        std::string_view Render(FrameGraph& frameGraph, const FrameContext& frameContext) override;
        void BuildUI() override;

        const char* GetName() const override { return "Hybrid"; }

    private:
        void DefineFrameGraphResources(FrameGraph& frameGraph, const FrameContext& frameContext);

    private:
        ClearPass m_clearPass{};
        MeshPass m_meshPass{};
        BuildTlasPass m_buildTlasPass{};
    };
}
