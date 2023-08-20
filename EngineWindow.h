#pragma once

class EngineWindow
{

public:
	EngineWindow(LPCWSTR windowName, UINT width, UINT height);
	~EngineWindow();

private:
	HWND m_window = nullptr;

	HINSTANCE m_appInstance;

	LPCWSTR m_windowName = L"CARDINAL WINDOW";

	UINT m_width = 1280;
	UINT m_height = 720;

public:

	HWND GetWindow() { return m_window; }
	HWND GetWindow(HWND* window) { *window = m_window; }

	void GetWindowDimensions(UINT* width, UINT* height);

private:

	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

