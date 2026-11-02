#include <h2/ui/userInterfaceManager.h>

#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>

#include <h2/ui/panels.h>

namespace Hydrogen
{
    UserInterfaceManager::~UserInterfaceManager()
    {
        if (ImGui::GetCurrentContext() == nullptr)
        {
            return;
        }

        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }

    void UserInterfaceManager::Initialize(HWND hWnd, const std::string& settingsPath)
    {
        m_settingsPath = settingsPath;

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();
        style.Colors[ImGuiCol_WindowBg].w = 0.6f;
        style.FontScaleMain = 1.25f;

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        if (!m_settingsPath.empty())
        {
            io.IniFilename = m_settingsPath.c_str();
        }

        ImGui_ImplWin32_Init(hWnd);
    }

    void UserInterfaceManager::BeginFrame()
    {
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }

    void UserInterfaceManager::DrawMainMenuBar()
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

    void UserInterfaceManager::Draw(UserInterfaceContext& context)
    {
        if (!m_visible)
        {
            return;
        }

        context.pSelection = &m_selection;

        DrawMainMenuBar();
        ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_PassthruCentralNode);

        for (auto& pPanel : m_panels)
        {
            if (!pPanel->IsOpen())
            {
                continue;
            }
            pPanel->Draw(context);
        }
    }

    ImDrawData* UserInterfaceManager::EndFrame()
    {
        ImGui::Render();
        return ImGui::GetDrawData();
    }

    bool UserInterfaceManager::WantsMouseCapture() const
    {
        return ImGui::GetIO().WantCaptureMouse;
    }

    bool UserInterfaceManager::WantsKeyboardCapture() const
    {
        return ImGui::GetIO().WantCaptureKeyboard;
    }
} // namespace Hydrogen
