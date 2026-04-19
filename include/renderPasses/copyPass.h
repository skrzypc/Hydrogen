#pragma once

#include <string>
#include "renderPass.h"
#include "frameGraphBuilder.h"

namespace Hydrogen
{
	class CopyPass : public IRenderPass
	{
	public:
		std::string src;
		std::string dst;

		void Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler) override {}

		void Setup(FGBuilder& builder) override
		{
			m_srcHandle = builder.Read(src, FGAccess::Input::CopySrc);
			m_dstHandle = builder.Write(dst, FGAccess::Output::CopyDst);
		}

		void Execute(FGExecuteContext& ctx, GraphicsContext& gfx) override;

	private:
		FGResourceHandle m_srcHandle{};
		FGResourceHandle m_dstHandle{};
	};
}
