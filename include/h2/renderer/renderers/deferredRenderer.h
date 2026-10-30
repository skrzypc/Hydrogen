#pragma once

#include <h2/renderer/renderers/renderer.h>
#include <h2/renderer/passes/gBufferPass.h>
#include <h2/renderer/passes/lightingPass.h>
#include <h2/renderer/passes/tonemapPass.h>

namespace Hydrogen
{
    class DeferredRenderer : public IRenderer
    {
    public:
        void Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler, uint32 renderWidth,
                        uint32 renderHeight) override;
        void Shutdown() override;

        std::string_view Render(FrameGraph& frameGraph, const FrameContext& frameContext) override;
        void BuildUI() override;

        const char* GetName() const override
        {
            return "Deferred";
        }

    private:
        void DefineFrameGraphResources(FrameGraph& frameGraph, const FrameContext& frameContext);

    private:
        GBufferPass m_gBufferPass{};
        LightingPass m_lightingPass{};
        TonemapPass m_tonemapPass{};
    };
} // namespace Hydrogen
