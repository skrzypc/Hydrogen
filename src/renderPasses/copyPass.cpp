#include "graphicsContext.h"
#include "renderPasses/copyPass.h"

namespace Hydrogen
{
	void CopyPass::Execute(FGExecuteContext& ctx, GraphicsContext& gfx)
	{
		gfx.CmdList()->CopyResource(ctx.GetResource(dst), ctx.GetResource(src));
	}
}
