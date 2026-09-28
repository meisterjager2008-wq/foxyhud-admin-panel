#include "foxyhud/renderers/dx11.h"

#include "imgui.h"
#include "imgui_impl_dx11.h"

namespace foxy::dx11 {

bool Init(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context, float ui_scale) {
    return win32::Init(hwnd, ui_scale) && ImGui_ImplDX11_Init(device, context);
}

void NewFrame() {
    ImGui_ImplDX11_NewFrame();
    win32::NewFrame();
    ImGui::NewFrame();
}

void Render() {
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void Shutdown() {
    ImGui_ImplDX11_Shutdown();
    win32::Shutdown();
}

} // namespace foxy::dx11
