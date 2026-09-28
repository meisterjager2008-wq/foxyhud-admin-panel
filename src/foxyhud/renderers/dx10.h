#pragma once

// DirectX 10 support. Per frame:
//     foxy::dx10::NewFrame();
//     panel.Render();
//     ...draw your scene, keep your back buffer bound (OMSetRenderTargets)...
//     foxy::dx10::Render();
//     swap_chain->Present(...);

#include "foxyhud/renderers/win32.h"

struct ID3D10Device;

namespace foxy::dx10 {

bool Init(HWND hwnd, ID3D10Device* device, float ui_scale = 0.0f);
void NewFrame();
void Render();  // draws into the currently bound render target
void Shutdown();

} // namespace foxy::dx10
