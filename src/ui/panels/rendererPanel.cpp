#include "ui/panels.h"

#include "ui/uiContext.h"

namespace Hydrogen
{
    void RendererPanel::Draw(UiContext& context)
    {
        if (context.fnBuildRendererUi)
        {
            context.fnBuildRendererUi();
        }
    }
}
