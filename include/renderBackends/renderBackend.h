#pragma once

#include <string_view>

#include "basicTypes.h"
#include "shaderInterop.h"
#include "frameContext.h"

namespace Hydrogen
{
    class GpuDevice;
    class ShaderCompiler;
    class FrameGraph;

    enum class eRenderBackendType : uint8
    {
        Deferred = 0,
        RayTracing,
        Count
    };

    class IRenderBackend
    {
    public:
        virtual ~IRenderBackend() = default;

        virtual void Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler) = 0;
        virtual void Shutdown() = 0;

        virtual std::string_view Render(FrameGraph& frameGraph, const FrameContext& frameContext) = 0;

        // TODO: consider per-backend FrameData struct if backends diverge significantly
        virtual void FillFrameData(FrameData& frameData) {}

        virtual void BuildUI() {}

        virtual const char* GetName() const = 0;

    protected:
        GpuDevice* m_pDevice = nullptr;
    };
}
