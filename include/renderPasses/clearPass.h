#pragma once

#include <array>
#include <string>
#include "renderPass.h"
#include "frameGraphBuilder.h"

namespace Hydrogen
{
	class ClearPass : public IRenderPass
	{
	public:
		std::string target = "";
		std::array<float32, 4> clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };

		void Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler) override {}

		void Setup(FGBuilder& builder) override
		{
			builder.Write(target, FGAccess::Write::RenderTarget);
		}

		void Execute(FGExecuteContext& fgExecuteContext, GraphicsContext& graphicsContext) override;
	};
}
