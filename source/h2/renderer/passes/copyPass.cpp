#include <h2/renderer/gpu/commands/graphicsContext.h>
#include <h2/renderer/passes/copyPass.h>

namespace Hydrogen
{
    void CopyPass::Execute(FGExecuteContext& fgExecuteContext, GraphicsContext& graphicsContext)
    {
        graphicsContext.CmdList()->CopyResource(fgExecuteContext.GetResource(dst), fgExecuteContext.GetResource(src));
    }
} // namespace Hydrogen
