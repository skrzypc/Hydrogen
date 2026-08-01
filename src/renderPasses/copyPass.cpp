#include "graphicsContext.h"
#include "renderPasses/copyPass.h"

namespace Hydrogen
{
	void CopyPass::Execute(FGExecuteContext& fgExecuteContext, GraphicsContext& graphicsContext)
	{
		graphicsContext.CmdList()->CopyResource(fgExecuteContext.GetResource(dst), fgExecuteContext.GetResource(src));
	}
}
