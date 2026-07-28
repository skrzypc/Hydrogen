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
		FGResourceHandle tlasHandle{};
		std::string outputTarget = "";

		void Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler) override;
		void Setup(FGBuilder& builder) override;
		void Execute(FGExecuteContext& ctx, GraphicsContext& gfx) override;

		uint32 GetOutputUavIndex() const { return m_outputUav.index; }

	private:
		GpuDevice* m_pDevice = nullptr;

		RaytracingPipelineState m_raytracingPso{};
		ShaderTable m_shaderTable{};

		UnorderedAccessViewHandle m_outputUav{};

		FGResourceHandle m_outputHandle{};
		uint32 m_width = 0;
		uint32 m_height = 0;
	};
}
