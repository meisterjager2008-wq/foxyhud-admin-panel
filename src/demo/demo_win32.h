#pragma once

// Window + message loop shared by the DirectX demos (not needed in your game,
// which already has its own window).

#include <windows.h>

#include "imgui_impl_win32.h"

namespace demo {

struct Window {
    HWND        hwnd = nullptr;
    WNDCLASSEXW wc = {};
    float       scale = 1.0f;  // monitor DPI scale (1.0 = 100%)
};

inline bool CreateAppWindow(Window& w, const wchar_t* title, WNDPROC proc) {
    ImGui_ImplWin32_EnableDpiAwareness();
    w.scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY));
    w.wc = {sizeof(WNDCLASSEXW), CS_CLASSDC, proc, 0L, 0L, ::GetModuleHandleW(nullptr), nullptr,
            ::LoadCursorW(nullptr, reinterpret_cast<LPCWSTR>(IDC_ARROW)), nullptr, nullptr, L"FoxyHUDDemo", nullptr};
    ::RegisterClassExW(&w.wc);
    w.hwnd = ::CreateWindowW(w.wc.lpszClassName, title, WS_OVERLAPPEDWINDOW, 100, 100, static_cast<int>(1280 * w.scale),
                             static_cast<int>(800 * w.scale), nullptr, nullptr, w.wc.hInstance, nullptr);
    return w.hwnd != nullptr;
}

inline void ShowAppWindow(const Window& w) {
    ::ShowWindow(w.hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(w.hwnd);
}

inline void DestroyAppWindow(Window& w) {
    if (w.hwnd)
        ::DestroyWindow(w.hwnd);
    ::UnregisterClassW(w.wc.lpszClassName, w.wc.hInstance);
}

// Handles pending window messages. Returns false once the window was closed.
inline bool PumpMessages() {
    bool running = true;
    MSG msg;
    while (::PeekMessageW(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
        ::TranslateMessage(&msg);
        ::DispatchMessageW(&msg);
        if (msg.message == WM_QUIT)
            running = false;
    }
    return running;
}

// Message handling common to every demo, after its own WM_SIZE handling.
inline LRESULT DefaultWindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
    case WM_SYSCOMMAND:
        if ((wparam & 0xfff0) == SC_KEYMENU)  // no ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hwnd, msg, wparam, lparam);
}

} // namespace demo
