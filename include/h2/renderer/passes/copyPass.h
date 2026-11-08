#pragma once

#include <string>
#include <h2/renderer/passes/renderPass.h>
#include <h2/renderer/framegraph/frameGraphBuilder.h>

namespace Hydrogen
{
    class CopyPass : public IRenderPass
    {
    public:
        std::string input;
        std::string output;

        void Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler) override {}

        void Setup(FGBuilder& builder) override
        {
            builder.Read(input, FGAccess::Read::CopySrc);
            builder.Write(output, FGAccess::Write::CopyDst);
        }

        void Execute(FGExecuteContext& fgExecuteContext, GraphicsContext& graphicsContext) override;
    };
} // namespace Hydrogen
