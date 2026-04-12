#pragma once

#include <array>
#include "renderPass.h"

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

		void Execute(FGExecuteContext& ctx, ID3D12GraphicsCommandList7* cmd) override
		{
			cmd->ClearRenderTargetView(ctx.GetRTV(m_handle), clearColor.data(), 0, nullptr);
		}

	private:
		FGResourceHandle m_handle{};
	};
}
