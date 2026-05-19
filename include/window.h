#pragma once

#include <Windows.h>
#include <string>
#include <optional>
#include <bitset>

#include <basicTypes.h>

namespace Hydrogen
{
    class Window
    {
    public:
        Window() = default;
        ~Window();
        Window(const Window&) = delete;
        const Window& operator=(const Window&) = delete;
        Window(Window&&) noexcept = default;
        Window& operator=(Window&&) noexcept = default;

		void Create(uint32 width, uint32 height, const std::wstring_view& windowTitle, const std::wstring_view& windowClassName = L"HydrogenWindowClass");

        void Resize(uint32 newWidth, uint32 newHeight);

        HWND GetHandle() const { return m_hwnd; }

		uint32 GetWidth() const { return m_width; }
		uint32 GetHeight() const { return m_height; }

        std::optional<int32> ProcessMessages();

        bool IsKeyDown(uint8 keyCode) const;
        bool IsKeyJustPressed(uint8 keyCode) const;
        bool IsKeyJustReleased(uint8 keyCode) const;

        bool IsLeftMouseDown() const;
        bool IsLeftMouseJustPressed() const;
        bool IsLeftMouseJustReleased() const;

        bool IsMiddleMouseDown() const;
        bool IsMiddleMouseJustPressed() const;
        bool IsMiddleMouseJustReleased() const;

        bool IsRightMouseDown() const;
        bool IsRightMouseJustPressed() const;
        bool IsRightMouseJustReleased() const;

        float32 GetMouseDeltaX() const;
        float32 GetMouseDeltaY() const;
        float32 GetMousePosX() const;
        float32 GetMousePosY() const;
        float32 GetScrollDelta() const;

    private:
        struct InputState
        {
            std::bitset<256> keys;
            bool leftMouseDown = false;
            bool middleMouseDown = false;
            bool rightMouseDown = false;
            float32 mouseDeltaX = 0.0f;
            float32 mouseDeltaY = 0.0f;
            float32 mousePosX = 0.0f;
            float32 mousePosY = 0.0f;
            float32 scrollDelta = 0.0f;
        };

        static LRESULT CALLBACK WindowProcedureSetup(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        static LRESULT CALLBACK WindowProcedurePassthrough(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    private:
        HWND m_hwnd{};
        HINSTANCE m_hInstance{};
        ATOM m_atom{};

		bool m_wasResized = false;
        uint32 m_width = 0u;
        uint32 m_height = 0u;

        InputState m_currentInput{};
        InputState m_previousInput{};
    };
}