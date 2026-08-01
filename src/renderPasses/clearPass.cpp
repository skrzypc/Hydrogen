#include "renderPasses/clearPass.h"
#include "graphicsContext.h"

namespace Hydrogen
{
	void ClearPass::Execute(FGExecuteContext& fgExecuteContext, GraphicsContext& graphicsContext)
	{
		graphicsContext.CmdList()->ClearRenderTargetView(fgExecuteContext.GetRTV(target), clearColor.data(), 0, nullptr);
	}
}
