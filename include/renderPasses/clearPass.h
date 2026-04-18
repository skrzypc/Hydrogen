#pragma once

#include <array>
#include "renderPass.h"
#include "frameGraphBuilder.h"

namespace Hydrogen
{
	class ClearPass : public IRenderPass
	{
	public:
		eFrameResource target = eFrameResource::Backbuffer;
		std::array<float, 4> clearColor = { 0.1f, 0.1f, 0.1f, 1.0f };

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
