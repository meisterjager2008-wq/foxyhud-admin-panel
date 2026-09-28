#pragma once

// DirectX 9 support. Per frame:
//     foxy::dx9::NewFrame();
//     panel.Render();
//     device->BeginScene();  ...your scene...  foxy::dx9::Render();  device->EndScene();
//     device->Present(...);

#include "foxyhud/renderers/win32.h"

struct IDirect3DDevice9;

namespace foxy::dx9 {

bool Init(HWND hwnd, IDirect3DDevice9* device, float ui_scale = 0.0f);
void NewFrame();
void Render();          // between BeginScene() and EndScene()
void OnDeviceLost();    // call right before IDirect3DDevice9::Reset()
void OnDeviceReset();   // call after Reset() succeeded
void Shutdown();

} // namespace foxy::dx9
