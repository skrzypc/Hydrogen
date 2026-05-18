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
			m_handle = builder.Write(target, FGAccess::Output::RenderTarget);
		}

		void Execute(FGExecuteContext& ctx, GraphicsContext& gfx) override;

	private:
		FGResourceHandle m_handle{};
	};
}
