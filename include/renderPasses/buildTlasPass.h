#pragma once

#include "renderPass.h"
#include "frameGraphBuilder.h"

namespace Hydrogen
{
    class GpuScene;

    class BuildTlasPass : public IRenderPass
    {
    public:
        GpuScene* pScene = nullptr;

        void Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler) override;
        void Setup(FGBuilder& builder) override;
        void Execute(FGExecuteContext& ctx, GraphicsContext& gfx) override;
    };
}
