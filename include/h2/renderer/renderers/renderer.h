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

    enum class eRendererType : uint8
    {
        Deferred = 0,
        RayTracing,
        Count
    };

    class IRenderer
    {
    public:
        virtual ~IRenderer() = default;

        virtual void Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler, uint32 renderWidth,
                                uint32 renderHeight) = 0;
        virtual void Shutdown() = 0;

        virtual std::string_view Render(FrameGraph& frameGraph, const FrameContext& frameContext) = 0;

        // TODO: consider per-renderer FrameData struct if renderers diverge significantly
        virtual void FillFrameData(FrameData& frameData) {}

        virtual void BuildUI() {}

        virtual const char* GetName() const = 0;

    protected:
        GpuDevice* m_pDevice = nullptr;
    };
} // namespace Hydrogen
