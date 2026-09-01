#pragma once

#include <memory>
#include <string>

#include "renderPass.h"
#include "frameGraphBuilder.h"
#include "raytracingPipelineState.h"
#include "shaderTable.h"
#include "device.h"

namespace Hydrogen
{
	class RayTraceDispatchPass : public IRenderPass
	{
	public:
		struct PushConstants
		{
			uint32 tlasIndex = 0;
			uint32 outputUavIndex = 0;
			uint32 accumulationTargetUavIndex = 0;
			uint32 accumulatedFramesCount = 0;
		};

		std::string outputTarget = "";
		bool resetAccumulation = false;

		void Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler) override;
		void Setup(FGBuilder& builder) override;
		void Execute(FGExecuteContext& fgExecuteContext, GraphicsContext& graphicsContext) override;

		Texture* GetAccumulationTarget() const { return m_pAccumulationTarget.get(); }

	private:
		GpuDevice* m_pDevice = nullptr;

		RaytracingPipelineState m_raytracingPso{};
		ShaderTable m_shaderTable{};

		std::unique_ptr<Texture> m_pAccumulationTarget{};

		uint32 m_accumulatedFramesCount = 0;

		uint32 m_width = 0;
		uint32 m_height = 0;
	};
}
