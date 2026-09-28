#include "foxyhud/renderers/dx9.h"

#include "imgui.h"
#include "imgui_impl_dx9.h"

namespace foxy::dx9 {

bool Init(HWND hwnd, IDirect3DDevice9* device, float ui_scale) {
    return win32::Init(hwnd, ui_scale) && ImGui_ImplDX9_Init(device);
}

void NewFrame() {
    ImGui_ImplDX9_NewFrame();
    win32::NewFrame();
    ImGui::NewFrame();
}

void Render() {
    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
}

void OnDeviceLost() { ImGui_ImplDX9_InvalidateDeviceObjects(); }

void OnDeviceReset() { ImGui_ImplDX9_CreateDeviceObjects(); }

void Shutdown() {
    ImGui_ImplDX9_Shutdown();
    win32::Shutdown();
}

} // namespace foxy::dx9
