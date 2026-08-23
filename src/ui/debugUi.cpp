#include "ui/debugUi.h"

#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>

#include "ui/panels.h"

namespace Hydrogen
{
    DebugUi::DebugUi()
    {
        m_panels.push_back(std::make_unique<ScenePanel>());
        m_panels.push_back(std::make_unique<InspectorPanel>());
        m_panels.push_back(std::make_unique<StatsPanel>());
        m_panels.push_back(std::make_unique<RendererPanel>());
    }

    void DebugUi::BeginFrame()
    {
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }

    void DebugUi::DrawMainMenuBar()
    {
        if (!ImGui::BeginMainMenuBar())
        {
            return;
        }

        if (ImGui::BeginMenu("Panels"))
        {
            for (auto& pPanel : m_panels)
            {
                bool open = pPanel->IsOpen();
                if (ImGui::MenuItem(pPanel->GetName(), nullptr, &open))
                {
                    pPanel->SetOpen(open);
                }
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    void DebugUi::Draw(UiContext& context)
    {
        context.selection = m_selection;

        DrawMainMenuBar();

        for (auto& pPanel : m_panels)
        {
            if (!pPanel->IsOpen())
            {
                continue;
            }
            pPanel->Draw(context);
        }

        m_selection = context.selection;
    }

    ImDrawData* DebugUi::EndFrame()
    {
        ImGui::Render();
        return ImGui::GetDrawData();
    }

    bool DebugUi::WantsMouseCapture() const
    {
        return ImGui::GetIO().WantCaptureMouse;
    }

    bool DebugUi::WantsKeyboardCapture() const
    {
        return ImGui::GetIO().WantCaptureKeyboard;
    }
}
