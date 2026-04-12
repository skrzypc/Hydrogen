#pragma once

#include <d3d12.h>

namespace Hydrogen
{
	class GpuDevice;
	class FGBuilder;
	class ShaderCompiler;
	class FGExecuteContext;

	class IRenderPass
	{
	public:
		virtual ~IRenderPass() = default;
		virtual void Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler) = 0;
		virtual void Setup(FGBuilder& builder) = 0;
		virtual void Execute(FGExecuteContext& ctx, ID3D12GraphicsCommandList7* cmd) = 0;
	};
}