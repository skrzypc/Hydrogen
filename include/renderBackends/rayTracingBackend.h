#pragma once

#include "renderBackend.h"
#include "renderPasses/buildTlasPass.h"
#include "renderPasses/rayTraceDispatchPass.h"

namespace Hydrogen
{
    class RayTracingBackend : public IRenderBackend
    {
    public:
        void Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler, GpuScene& gpuScene) override;
        void Shutdown() override;

        std::string_view Render(FrameGraph& frameGraph, const RenderScene& scene, const Texture::Desc& outputDesc) override;
        void BuildUI() override;

        const char* GetName() const override { return "RayTracing"; }

    private:
        void DefineFrameGraphResources(FrameGraph& frameGraph, const RenderScene& scene, const Texture::Desc& outputDesc);

    private:
        GpuScene* m_pGpuScene = nullptr;

        BuildTlasPass m_buildTlasPass{};
        RayTraceDispatchPass m_rayTraceDispatchPass{};
    };
}
