#pragma once

#include <h2/renderer/renderers/renderer.h>
#include <h2/renderer/passes/buildTlasPass.h>
#include <h2/renderer/passes/rayTraceDispatchPass.h>
#include <h2/renderer/passes/tonemapPass.h>

namespace Hydrogen
{
    class RayTracingRenderer : public IRenderer
    {
    public:
        void Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler, uint32 renderWidth,
                        uint32 renderHeight) override;
        void Shutdown() override;

        std::string_view Render(FrameGraph& frameGraph, const FrameContext& frameContext) override;
        void BuildUI() override;

        const char* GetName() const override
        {
            return "RayTracing";
        }

    private:
        void DefineFrameGraphResources(FrameGraph& frameGraph, const FrameContext& frameContext);

    private:
        BuildTlasPass m_buildTlasPass{};
        RayTraceDispatchPass m_rayTraceDispatchPass{};
        TonemapPass m_tonemapPass{};
    };
} // namespace Hydrogen
