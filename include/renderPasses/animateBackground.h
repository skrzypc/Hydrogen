#pragma once

#include <DirectXMath.h>

#include "renderPass.h"
#include "pipelineState.h"
#include "frameGraphStructs.h"
#include "basicTypes.h"

namespace Hydrogen
{
	class AnimateBackgroundPass : public IRenderPass
	{
	public:
		struct PassData
		{
			DirectX::XMFLOAT4 colorTint = { 1.0f, 1.0f, 1.0f, 1.0f };
		};

		std::string target = "";

		void Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler) override;
		void Setup(FGBuilder& builder) override;
		void Execute(FGExecuteContext& ctx, GraphicsContext& gfx) override;

	private:
		PipelineState m_pso{}; // TODO: Later move to PSO cache.
		
		FGResourceHandle m_targetHandle{};

		uint32 m_width = 0;
		uint32 m_height = 0;

		float m_time = 0.0f;
		PassData m_passData{};
	};
}
