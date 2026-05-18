#pragma once

#include <span>
#include <string>
#include "renderPass.h"
#include "pipelineState.h"
#include "frameGraphStructs.h"
#include "renderScene.h"
#include "basicTypes.h"

namespace Hydrogen
{
	class GpuScene;

	class MeshPass : public IRenderPass
	{
	public:
		struct PushConstants
		{
			uint32 transformIndex;
			uint32 baseVertex;
		};

		std::string target;
		std::string depthTarget;
		const GpuScene* pScene = nullptr;
		std::span<const RenderObject> renderObjects{};

		void Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler) override;
		void Setup(FGBuilder& builder) override;
		void Execute(FGExecuteContext& ctx, GraphicsContext& gfx) override;

	private:
		PipelineState m_pso{};
		FGResourceHandle m_targetHandle{};
		FGResourceHandle m_depthHandle{};
		uint32 m_width = 0;
		uint32 m_height = 0;
	};
}
