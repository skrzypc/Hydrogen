#include "ui/panels.h"

#include <imgui.h>

#include "ui/uiContext.h"

namespace Hydrogen
{
    void StatsPanel::Draw(UiContext& context)
    {
        ImGui::Begin(GetName());

        ImGui::Text("FPS: %.1f (%.2f ms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);

        ImGui::End();
    }
}
