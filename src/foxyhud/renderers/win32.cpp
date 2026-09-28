#include "foxyhud/renderers/win32.h"

#include "foxyhud/theme.h"
#include "imgui.h"
#include "imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

namespace foxy::win32 {

bool Init(HWND hwnd, float ui_scale) {
    if (!ImGui::GetCurrentContext()) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
    }
    if (!theme::GetFonts().body) {
        const float scale = ui_scale > 0.0f ? ui_scale : ImGui_ImplWin32_GetDpiScaleForHwnd(hwnd);
        theme::LoadFonts(scale);
        theme::Apply(scale);
    }
    return ImGui_ImplWin32_Init(hwnd);
}

void NewFrame() { ImGui_ImplWin32_NewFrame(); }

void Shutdown() { ImGui_ImplWin32_Shutdown(); }

bool HandleMessage(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    return ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam) != 0;
}

} // namespace foxy::win32
