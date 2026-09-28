#pragma once

// Win32 part shared by the DirectX helpers (dx9.h ... dx12.h). You normally
// only call HandleMessage() yourself; Init/NewFrame/Shutdown are called by the
// DirectX helper you use.

#include <windows.h>

namespace foxy::win32 {

// Creates the ImGui context (unless one exists), loads the menu's fonts and
// theme scaled for the window's DPI (or `ui_scale` if > 0) and connects ImGui
// to the window.
bool Init(HWND hwnd, float ui_scale = 0.0f);
void NewFrame();
void Shutdown();

// Call first thing in your window procedure:
//     if (foxy::win32::HandleMessage(hwnd, msg, wparam, lparam)) return true;
bool HandleMessage(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

} // namespace foxy::win32
