#include "foxyhud/renderers/dx10.h"

#include "imgui.h"
#include "imgui_impl_dx10.h"

namespace foxy::dx10 {

bool Init(HWND hwnd, ID3D10Device* device, float ui_scale) {
    return win32::Init(hwnd, ui_scale) && ImGui_ImplDX10_Init(device);
}

void NewFrame() {
    ImGui_ImplDX10_NewFrame();
    win32::NewFrame();
    ImGui::NewFrame();
}

void Render() {
    ImGui::Render();
    ImGui_ImplDX10_RenderDrawData(ImGui::GetDrawData());
}

void Shutdown() {
    ImGui_ImplDX10_Shutdown();
    win32::Shutdown();
}

} // namespace foxy::dx10
