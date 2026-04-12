#pragma once

#include "renderPass.h"
#include "pipelineState.h"

namespace Hydrogen
{
	class TestTrianglePass : public IRenderPass
	{
	public:
		void Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler) override;
		void Setup(FGBuilder& builder) override;
		void Execute(FGExecuteContext& ctx, ID3D12GraphicsCommandList7* cmd) override;

	private:
		PipelineState m_pso{};
		FGResourceHandle m_backBuffer{};
		uint32 m_width = 0;
		uint32 m_height = 0;
	};
}
