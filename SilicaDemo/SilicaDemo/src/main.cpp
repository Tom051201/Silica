#include <Windows.h>
#include <iostream>

#include "DemoApp.h"

#include "Silica/backends/SilicaImplWin32.h"

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	const wchar_t CLASS_NAME[] = L"SilicaDemoWindowClass";

	WNDCLASSEXW wc = {};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);

	RegisterClassExW(&wc);

	// Create the window
	HWND hwnd = CreateWindowExW(
		0,
		CLASS_NAME,
		L"Silica UI - Demo Sandbox",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		1280, 720,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	if (hwnd == NULL) {
		return 0;
	}

	ShowWindow(hwnd, nCmdShow);

	DemoApp app;
	if (!app.initialize(hwnd, 1280, 720)) {
		return -1;
	}

	SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&app));

	// -- Message Loop --
	MSG msg = {};
	bool isRunning = true;

	while (isRunning) {
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				isRunning = false;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (!isRunning) break;

		app.render();

	}

	app.cleanup();

	return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	DemoApp* app = reinterpret_cast<DemoApp*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

	if (app && app->getUIRoot()) {
		if (Silica::ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam, app->getUIRoot())) {
			return 1;
		}
	}

	switch (uMsg) {
		case WM_DESTROY: {
			PostQuitMessage(0);
			return 0;
		}
		case WM_SIZE: {
			int width = LOWORD(lParam);
			int height = HIWORD(lParam);

			if (app) {
				app->resize(width, height);
			}
			return 0;
		}
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

