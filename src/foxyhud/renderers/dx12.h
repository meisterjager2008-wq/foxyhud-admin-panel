#pragma once

// DirectX 12 support. Per frame:
//     foxy::dx12::NewFrame();
//     panel.Render();
//     ...record your scene; back buffer in RENDER_TARGET state and bound with OMSetRenderTargets...
//     foxy::dx12::Render(command_list);
//     ...transition to PRESENT, close, execute, Present...
//
// The helper creates its own small shader-visible SRV heap for the menu's
// textures and binds it on the command list inside Render().

#include <d3d12.h>

#include "foxyhud/renderers/win32.h"

namespace foxy::dx12 {

// frames_in_flight: how many frames your renderer keeps in flight (usually 2 or 3).
// rtv_format: format of the render target the menu is drawn into (your back buffer).
bool Init(HWND hwnd, ID3D12Device* device, ID3D12CommandQueue* queue, int frames_in_flight, DXGI_FORMAT rtv_format,
          float ui_scale = 0.0f);
void NewFrame();
void Render(ID3D12GraphicsCommandList* command_list);
void Shutdown();  // make sure the GPU is idle first

} // namespace foxy::dx12
