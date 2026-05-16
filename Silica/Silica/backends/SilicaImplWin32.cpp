#include "SilicaImplWin32.h"

#include <windowsx.h>

#include "../include/Renderer.h"

namespace Silica {

	struct BackendStateWin32 {
		HWND hwnd = nullptr;
		float clientWidth = 0.0f;
		float clientHeight = 0.0f;
	};

	static BackendStateWin32 s_state;

	bool ImplWin32_init(HWND hwnd) {
		s_state.hwnd = hwnd;

		RECT rect;
		GetClientRect(hwnd, &rect);
		s_state.clientWidth = static_cast<float>(rect.right - rect.left);
		s_state.clientHeight = static_cast<float>(rect.bottom - rect.top);

		return true;
	}

	void ImplWin32_shutdown() {
		s_state.hwnd = nullptr;
		s_state.clientHeight = 0.0f;
		s_state.clientHeight = 0.0f;
	}

	bool ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, WidgetPtr rootWidget) {
		if (!rootWidget) return false;

		switch (msg) {
			case WM_SIZE: {
				s_state.clientWidth = static_cast<float>(LOWORD(lParam));
				s_state.clientHeight = static_cast<float>(HIWORD(lParam));
				return false;
			}
			case WM_MOUSEMOVE: {
				float x = static_cast<float>(GET_X_LPARAM(lParam));
				float y = static_cast<float>(GET_Y_LPARAM(lParam));

				Renderer::processMouseMove(rootWidget, s_state.clientWidth, s_state.clientHeight, x, y);
				return true;
			}
			case WM_LBUTTONDOWN: {
				float x = static_cast<float>(GET_X_LPARAM(lParam));
				float y = static_cast<float>(GET_Y_LPARAM(lParam));

				Silica::Renderer::processMouseClick(rootWidget, s_state.clientWidth, s_state.clientHeight, x, y);
				return true;
			}
			case WM_LBUTTONUP: {
				float x = static_cast<float>(GET_X_LPARAM(lParam));
				float y = static_cast<float>(GET_Y_LPARAM(lParam));

				Silica::Renderer::processMouseUp(rootWidget, s_state.clientWidth, s_state.clientHeight, x, y);
				return true;
			}
		}

		return false;
	}

}
