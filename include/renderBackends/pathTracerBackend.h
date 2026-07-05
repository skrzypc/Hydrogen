#pragma once

#include "renderBackend.h"
#include "renderPasses/clearPass.h"
#include "renderPasses/buildTlasPass.h"

namespace Hydrogen
{
    class PathTracerBackend : public IRenderBackend
    {
    public:
        void Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler, GpuScene& gpuScene) override;
        void Shutdown() override;

        std::string_view Render(FrameGraph& frameGraph, const RenderScene& scene, const Texture::Desc& outputDesc) override;
        void FillFrameData(ShaderInterop::FrameData& frameData) override;
        void BuildUI() override;

        const char* GetName() const override { return "PathTracer"; }

    private:
        void DefineFrameGraphResources(FrameGraph& frameGraph, const RenderScene& scene, const Texture::Desc& outputDesc);

    private:
        GpuScene* m_pGpuScene = nullptr;

        ClearPass m_clearPass{};
        BuildTlasPass m_buildTlasPass{};
    };
}
