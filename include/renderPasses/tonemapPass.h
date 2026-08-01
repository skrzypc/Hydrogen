#pragma once

#include "renderPass.h"
#include "pipelineState.h"
#include "basicTypes.h"

namespace Hydrogen
{
	class TonemapPass : public IRenderPass
	{
	public:
		struct PushConstants
		{
			uint32 sceneColorIndex = 0;
			uint32 outputIndex = 0;
		};

		void Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler) override;
		void Setup(FGBuilder& builder) override;
		void Execute(FGExecuteContext& fgExecuteContext, GraphicsContext& graphicsContext) override;

	private:
		static constexpr uint32 ThreadGroupSize = 8;

		GpuDevice* m_pDevice = nullptr;
		PipelineState m_pso{};

		uint32 m_width = 0;
		uint32 m_height = 0;
	};
}
