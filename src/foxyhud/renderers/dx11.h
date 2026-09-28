#pragma once

// DirectX 11 support. Per frame:
//     foxy::dx11::NewFrame();
//     panel.Render();
//     ...draw your scene, keep your back buffer bound (OMSetRenderTargets)...
//     foxy::dx11::Render();
//     swap_chain->Present(...);

#include "foxyhud/renderers/win32.h"

struct ID3D11Device;
struct ID3D11DeviceContext;

namespace foxy::dx11 {

bool Init(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context, float ui_scale = 0.0f);
void NewFrame();
void Render();  // draws into the currently bound render target
void Shutdown();

} // namespace foxy::dx11
