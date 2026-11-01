#include <h2/ui/panels.h>

#include <imgui.h>

#include <h2/core/verifier.h>
#include <h2/renderer/renderer.h>
#include <h2/ui/userInterfaceContext.h>

namespace Hydrogen
{
    void RendererPanel::Draw(UserInterfaceContext& context)
    {
        H2_VERIFY(context.pRenderer != nullptr, "Renderer is missing from the UI context!");

        ImGui::Begin(GetName());

        context.rendererRequest = context.pRenderer->BuildUi();

        ImGui::End();
    }
} // namespace Hydrogen
