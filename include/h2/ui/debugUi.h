#pragma once

#include <memory>
#include <vector>

#include <h2/scene/entity.h>
#include <h2/ui/panel.h>
#include <h2/ui/uiContext.h>

struct ImDrawData;

namespace Hydrogen
{
    class DebugUi
    {
    public:
        DebugUi();
        ~DebugUi() = default;
        DebugUi(const DebugUi&) = delete;
        DebugUi& operator=(const DebugUi&) = delete;
        DebugUi(DebugUi&&) noexcept = default;
        DebugUi& operator=(DebugUi&&) noexcept = default;

        void BeginFrame();
        void Draw(UiContext& context);
        [[nodiscard]] ImDrawData* EndFrame();

        bool WantsMouseCapture() const;
        bool WantsKeyboardCapture() const;

    private:
        void DrawMainMenuBar();

        std::vector<std::unique_ptr<IPanel>> m_panels{};
        Entity m_selection{};
    };
} // namespace Hydrogen
