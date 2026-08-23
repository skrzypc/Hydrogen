#pragma once

namespace Hydrogen
{
    struct UiContext;

    class IPanel
    {
    public:
        virtual ~IPanel() = default;

        virtual void Draw(UiContext& context) = 0;
        virtual const char* GetName() const = 0;

        bool IsOpen() const { return m_open; }
        void SetOpen(bool open) { m_open = open; }

    protected:
        bool m_open = true;
    };
}
