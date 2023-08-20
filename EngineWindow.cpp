#include "cardinal_pch.h"
#include "cardinal.h"

#include "core.h"

EngineWindow::EngineWindow(LPCWSTR windowName, UINT width, UINT height): m_windowName(windowName), m_width(width), m_height(height)
{
	m_appInstance = GetModuleHandle(NULL);

	WNDCLASS windowClass = {};
	ZeroMemory(&windowClass, sizeof(WNDCLASS));
	windowClass.hInstance = m_appInstance;
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpszClassName = L"CARDINAL_WINDOW";
	windowClass.lpfnWndProc = EngineWindow::WindowProc;

	if (FAILED(RegisterClass(&windowClass))) {

		Logger::Error("FAILED TO REGISTER WINDOW CLASS");

		throw std::runtime_error("FAILED TO REGISTER WINDOW CLASS");
	}

	m_window = CreateWindowEx(NULL, windowClass.lpszClassName, m_windowName, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, m_appInstance, NULL);

	if (m_window == NULL) {

		Logger::Error("FAILED TO CREATE ENGINE WINDOW");

		throw std::runtime_error("FAILED TO CREATE ENGINE WINDOW");
	}

	ShowWindow(m_window, SW_SHOWDEFAULT);
}

EngineWindow::~EngineWindow()
{
	UnregisterClass(L"CARDINAL_WINDOW", m_appInstance);

	DestroyWindow(m_window);
}

void EngineWindow::GetWindowDimensions(UINT* width, UINT* height)
{
	*width = m_width;
	*height = m_height;
}

LRESULT CALLBACK EngineWindow::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_QUIT:
		PostQuitMessage(0);

		break;
	case WM_DESTROY:
		DestroyWindow(hWnd);

		break;

	case WM_SIZE:
	default:

		break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}