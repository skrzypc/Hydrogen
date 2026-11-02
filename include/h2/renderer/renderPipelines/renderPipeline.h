#pragma once

#include <string_view>

#include <h2/core/basicTypes.h>
#include <h2/shaders/shaderInterop.h>
#include <h2/renderer/frameContext.h>

namespace Hydrogen
{
    class GpuDevice;
    class ShaderCompiler;
    class FrameGraph;

    enum class eRenderPipelineType : uint8
    {
        Deferred = 0,
        RayTracing,
        Count
    };

    class IRenderPipeline
    {
    public:
        virtual ~IRenderPipeline() = default;

        virtual void Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler, uint32 renderWidth,
                                uint32 renderHeight) = 0;
        virtual void Shutdown() = 0;

        virtual std::string_view Render(FrameGraph& frameGraph, const FrameContext& frameContext) = 0;

        // TODO: consider per-pipeline FrameData struct if render pipelines diverge significantly
        virtual void FillFrameData(FrameData& frameData) {}

        virtual void BuildUI() {}

        virtual const char* GetName() const = 0;

    protected:
        GpuDevice* m_pDevice = nullptr;
    };
} // namespace Hydrogen
