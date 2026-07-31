#pragma once

#include <string>
#include "renderPass.h"
#include "frameGraphBuilder.h"

namespace Hydrogen
{
	class CopyPass : public IRenderPass
	{
	public:
		std::string src;
		std::string dst;

		void Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler) override {}

		void Setup(FGBuilder& builder) override
		{
			builder.Read(src, FGAccess::Read::CopySrc);
			builder.Write(dst, FGAccess::Write::CopyDst);
		}

		void Execute(FGExecuteContext& ctx, GraphicsContext& gfx) override;
	};
}
