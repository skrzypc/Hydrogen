#pragma once

#include <Windows.h>
#include <string>
#include <optional>

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

    private:
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
    };
}