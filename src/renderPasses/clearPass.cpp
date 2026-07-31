#include "renderPasses/clearPass.h"
#include "graphicsContext.h"

namespace Hydrogen
{
	void ClearPass::Execute(FGExecuteContext& ctx, GraphicsContext& gfx)
	{
		gfx.CmdList()->ClearRenderTargetView(ctx.GetRTV(target), clearColor.data(), 0, nullptr);
	}
}
