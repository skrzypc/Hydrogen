#pragma once

#include <string_view>

#include "basicTypes.h"
#include "renderScene.h"
#include "texture.h"

namespace Hydrogen
{
    class GpuDevice;
    class ShaderCompiler;
    class GpuScene;
    class FrameGraph;

    enum class eRenderBackendType : uint8
    {
        Raster = 0,
        Count
    };

    class IRenderBackend
    {
    public:
        virtual ~IRenderBackend() = default;

        virtual void Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler, GpuScene& gpuScene) = 0;
        virtual void Shutdown() = 0;

        virtual std::string_view Render(FrameGraph& frameGraph, const RenderScene& scene, const Texture::Desc& outputDesc) = 0;

        virtual void BuildUI() {}

        virtual const char* GetName() const = 0;
    };
}
