#pragma once

#include <Windows.h>

#include <memory>
#include <string>
#include <vector>

#include <h2/scene/entity.h>
#include <h2/ui/panels.h>
#include <h2/ui/userInterfaceContext.h>

struct ImDrawData;

namespace Hydrogen
{
    class UserInterfaceManager
    {
    public:
        UserInterfaceManager() = default;
        ~UserInterfaceManager();
        UserInterfaceManager(const UserInterfaceManager&) = delete;
        UserInterfaceManager& operator=(const UserInterfaceManager&) = delete;
        UserInterfaceManager(UserInterfaceManager&&) = delete;
        UserInterfaceManager& operator=(UserInterfaceManager&&) = delete;

        void Initialize(HWND hWnd, const std::string& settingsPath);

        template <typename TPanel, typename... TArgs> TPanel& AddPanel(TArgs&&... args)
        {
            auto pPanel = std::make_unique<TPanel>(std::forward<TArgs>(args)...);
            TPanel& panel = *pPanel;
            m_panels.push_back(std::move(pPanel));
            return panel;
        }

        void BeginFrame();
        void Draw(UserInterfaceContext& context);
        [[nodiscard]] ImDrawData* EndFrame();

        bool WantsMouseCapture() const;
        bool WantsKeyboardCapture() const;

        Entity& GetSelection() noexcept
        {
            return m_selection;
        }

        void SetVisible(bool visible) noexcept
        {
            m_visible = visible;
        }
        bool IsVisible() const noexcept
        {
            return m_visible;
        }

    private:
        void DrawMainMenuBar();

        std::vector<std::unique_ptr<IPanel>> m_panels{};
        Entity m_selection{};
        std::string m_settingsPath{};
        bool m_visible = true;
    };
} // namespace Hydrogen
