#pragma once

namespace Hydrogen
{
	class GpuDevice;
	class FGBuilder;
	class ShaderCompiler;
	class FGExecuteContext;
	class GraphicsContext;

	class IRenderPass
	{
	public:
		virtual ~IRenderPass() = default;
		virtual void Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler) = 0;
		virtual void Setup(FGBuilder& builder) = 0;
		virtual void Execute(FGExecuteContext& ctx, GraphicsContext& gfx) = 0;
	};
}