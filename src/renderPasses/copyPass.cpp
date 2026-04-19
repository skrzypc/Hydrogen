#include "graphicsContext.h"
#include "renderPasses/copyPass.h"

namespace Hydrogen
{
	void CopyPass::Execute(FGExecuteContext& ctx, GraphicsContext& gfx)
	{
		gfx.CmdList()->CopyResource(ctx.GetResource(m_dstHandle), ctx.GetResource(m_srcHandle));
	}
}
