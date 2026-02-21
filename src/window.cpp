
#include "window.h"
#include "logger.h"

namespace Hydrogen
{
	Window::~Window()
	{
		if (m_hwnd)
		{
			DestroyWindow(m_hwnd);
		}
	}

	void Window::Create(const uint32 width, const uint32 height, const std::wstring_view& windowTitle, const std::wstring_view& windowClassName)
	{
		m_width = width;
		m_height = height;
		m_hInstance = GetModuleHandleW(nullptr);

		const WNDCLASSEX wndClass
		{
			.cbSize = sizeof(WNDCLASSEX),
			.style = CS_OWNDC,
			.lpfnWndProc = Window::WindowProcedureSetup,
			.cbClsExtra = 0,
			.cbWndExtra = 0,
			.hInstance = m_hInstance,
			.hIcon = LoadIcon(nullptr, IDI_APPLICATION),
			.hCursor = LoadCursor(nullptr, IDC_ARROW),
			.hbrBackground = GetSysColorBrush(COLOR_BTNFACE),
			.lpszClassName = windowClassName.data(),
			.hIconSm = LoadIcon(nullptr, IDI_APPLICATION)
		};

		m_atom = RegisterClassEx(&wndClass);

		RECT adjustedWndSize{};
		adjustedWndSize.left = adjustedWndSize.top = 0;
		adjustedWndSize.right = m_width;
		adjustedWndSize.bottom = m_height;
		AdjustWindowRect(&adjustedWndSize, WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU, FALSE);

		m_hwnd = CreateWindowEx(
			0,
			windowClassName.data(),
			windowTitle.data(),
			WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | WS_SIZEBOX,
			100, 40,
			adjustedWndSize.right - adjustedWndSize.left,
			adjustedWndSize.bottom - adjustedWndSize.top,
			nullptr,
			nullptr,
			m_hInstance,
			this);

		if (!m_hwnd)
		{
			H2_ERROR(eLogLevel::Minimal, "Window creation failed!");

			return;
		}

		ShowWindow(m_hwnd, SW_SHOW);
		UpdateWindow(m_hwnd);

		ProcessMessages();
	}

	void Window::Resize(uint32 newWidth, uint32 newHeight)
	{
		m_width = newWidth;
		m_height = newHeight;
		m_wasResized = true;
	}

	LRESULT Window::WindowProcedureSetup(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if (uMsg == WM_NCCREATE)
		{
			CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
			Window* const pWindow = static_cast<Window*>(pCreate->lpCreateParams);

			SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWindow));
			SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&Window::WindowProcedurePassthrough));

			return pWindow->WindowProcedure(hWnd, uMsg, wParam, lParam);
		}

		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	LRESULT Window::WindowProcedurePassthrough(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		Window* const pWindow = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

		return pWindow->WindowProcedure(hWnd, uMsg, wParam, lParam);
	}

	LRESULT Window::WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case WM_ACTIVATEAPP:
		{
			break;
		}
		case WM_ACTIVATE:
		{
			break;
		}
		case WM_SIZE:
		{
			const uint32 newWidth = LOWORD(lParam);
			const uint32 newHeight = HIWORD(lParam);
			Resize(newWidth, newHeight);

			H2_INFO(eLogLevel::Verbose, "Window resize detected. New window size {}x{}", newWidth, newHeight);

			break;
		}
		case WM_INPUT:
		case WM_MOUSEMOVE:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_MOUSEWHEEL:
		case WM_XBUTTONDOWN:
		case WM_XBUTTONUP:
		case WM_MOUSEHOVER:
		{
			break;
		}
		case WM_MOUSEACTIVATE:
		{
			return MA_ACTIVATEANDEAT;
		}
		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYUP:
		{
			break;
		}
		case WM_SYSKEYDOWN:
		{
			break;
		}
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			return 0;
		}
		}

		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	std::optional<int32> Window::ProcessMessages()
	{
		MSG msg{};

		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				return static_cast<int32>(msg.wParam);
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		return {};
	}
}
