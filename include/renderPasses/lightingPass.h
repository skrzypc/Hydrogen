#pragma once

#include "renderPass.h"
#include "pipelineState.h"
#include "basicTypes.h"

namespace Hydrogen
{
	class LightingPass : public IRenderPass
	{
	public:
		// Scene referred radiance, so it has to hold values well above one.
		static constexpr DXGI_FORMAT SceneColorFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

		struct PushConstants
		{
			uint32 albedoIndex = 0;
			uint32 normalIndex = 0;
			uint32 roughnessMetalnessIndex = 0;
			uint32 depthIndex = 0;
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
