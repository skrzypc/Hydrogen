#include "renderPasses/clearPass.h"
#include "graphicsContext.h"

namespace Hydrogen
{
	void ClearPass::Execute(FGExecuteContext& ctx, GraphicsContext& gfx)
	{
		gfx.CmdList()->ClearRenderTargetView(ctx.GetRTV(m_handle), clearColor.data(), 0, nullptr);
	}
}
