
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

		RAWINPUTDEVICE rid{};
		rid.usUsagePage = 0x01;
		rid.usUsage = 0x02;
		rid.dwFlags = 0;
		rid.hwndTarget = m_hwnd;
		RegisterRawInputDevices(&rid, 1, sizeof(rid));

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
		{
			UINT size = sizeof(RAWINPUT);
			RAWINPUT raw{};
			if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &raw, &size, sizeof(RAWINPUTHEADER)) != static_cast<UINT>(-1))
			{
				if (raw.header.dwType == RIM_TYPEMOUSE)
				{
					m_currentInput.mouseDeltaX += static_cast<float32>(raw.data.mouse.lLastX);
					m_currentInput.mouseDeltaY += static_cast<float32>(raw.data.mouse.lLastY);
				}
			}
			break;
		}
		case WM_MOUSEMOVE:
		{
			m_currentInput.mousePosX = static_cast<float32>(LOWORD(lParam));
			m_currentInput.mousePosY = static_cast<float32>(HIWORD(lParam));
			break;
		}
		case WM_LBUTTONDOWN:
		{
			m_currentInput.leftMouseDown = true;
			break;
		}
		case WM_LBUTTONUP:
		{
			m_currentInput.leftMouseDown = false;
			break;
		}
		case WM_MBUTTONDOWN:
		{
			m_currentInput.middleMouseDown = true;
			break;
		}
		case WM_MBUTTONUP:
		{
			m_currentInput.middleMouseDown = false;
			break;
		}
		case WM_RBUTTONDOWN:
		{
			m_currentInput.rightMouseDown = true;
			SetCapture(hWnd);
			break;
		}
		case WM_RBUTTONUP:
		{
			m_currentInput.rightMouseDown = false;
			ReleaseCapture();
			break;
		}
		case WM_MOUSEWHEEL:
		{
			m_currentInput.scrollDelta += static_cast<float32>(GET_WHEEL_DELTA_WPARAM(wParam)) / static_cast<float32>(WHEEL_DELTA);
			break;
		}
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
		case WM_SYSKEYDOWN:
		{
			m_currentInput.keys[static_cast<uint8>(wParam)] = true;
			break;
		}
		case WM_KEYUP:
		case WM_SYSKEYUP:
		{
			m_currentInput.keys[static_cast<uint8>(wParam)] = false;
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

	bool Window::IsKeyDown(uint8 keyCode) const { return m_currentInput.keys[keyCode]; }
	bool Window::IsKeyJustPressed(uint8 keyCode) const { return m_currentInput.keys[keyCode] && !m_previousInput.keys[keyCode]; }
	bool Window::IsKeyJustReleased(uint8 keyCode) const { return !m_currentInput.keys[keyCode] && m_previousInput.keys[keyCode]; }

	bool Window::IsLeftMouseDown() const { return m_currentInput.leftMouseDown; }
	bool Window::IsLeftMouseJustPressed() const { return m_currentInput.leftMouseDown && !m_previousInput.leftMouseDown; }
	bool Window::IsLeftMouseJustReleased() const { return !m_currentInput.leftMouseDown && m_previousInput.leftMouseDown; }

	bool Window::IsMiddleMouseDown() const { return m_currentInput.middleMouseDown; }
	bool Window::IsMiddleMouseJustPressed() const { return m_currentInput.middleMouseDown && !m_previousInput.middleMouseDown; }
	bool Window::IsMiddleMouseJustReleased() const { return !m_currentInput.middleMouseDown && m_previousInput.middleMouseDown; }

	bool Window::IsRightMouseDown() const { return m_currentInput.rightMouseDown; }
	bool Window::IsRightMouseJustPressed() const { return m_currentInput.rightMouseDown && !m_previousInput.rightMouseDown; }
	bool Window::IsRightMouseJustReleased() const { return !m_currentInput.rightMouseDown && m_previousInput.rightMouseDown; }

	float32 Window::GetMouseDeltaX() const { return m_currentInput.mouseDeltaX; }
	float32 Window::GetMouseDeltaY() const { return m_currentInput.mouseDeltaY; }
	float32 Window::GetMousePosX() const { return m_currentInput.mousePosX; }
	float32 Window::GetMousePosY() const { return m_currentInput.mousePosY; }
	float32 Window::GetScrollDelta() const { return m_currentInput.scrollDelta; }

	std::optional<int32> Window::ProcessMessages()
	{
		m_previousInput = m_currentInput;
		m_currentInput.mouseDeltaX = 0.0f;
		m_currentInput.mouseDeltaY = 0.0f;
		m_currentInput.scrollDelta = 0.0f;

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
