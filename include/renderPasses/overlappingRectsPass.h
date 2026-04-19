#pragma once

#include <string>
#include <array>
#include "renderPass.h"
#include "pipelineState.h"
#include "frameGraphStructs.h"
#include "basicTypes.h"

namespace Hydrogen
{
	class OverlappingRectsPass : public IRenderPass
	{
	public:
		struct PushConstants
		{
			float rectMin[2];
			float rectMax[2];
			float color[4];
		};

		std::string target;

		void Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler) override;
		void Setup(FGBuilder& builder) override;
		void Execute(FGExecuteContext& ctx, GraphicsContext& gfx) override;

	private:
		PipelineState m_pso{};
		FGResourceHandle m_targetHandle{};
		uint32 m_width = 0;
		uint32 m_height = 0;
	};
}
