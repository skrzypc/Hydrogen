#pragma once

#include <h2/renderer/passes/renderPass.h>
#include <h2/renderer/framegraph/frameGraphBuilder.h>

namespace Hydrogen
{
    class GpuScene;

    class BuildTlasPass : public IRenderPass
    {
    public:
        const GpuScene* pScene = nullptr;

        void Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler) override;
        void Setup(FGBuilder& builder) override;
        void Execute(FGExecuteContext& fgExecuteContext, GraphicsContext& graphicsContext) override;
    };
} // namespace Hydrogen
