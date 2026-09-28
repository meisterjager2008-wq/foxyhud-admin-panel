// DirectX 10 demo: a window with the menu and nothing else. Press INSERT to toggle the menu.
// Device setup follows Dear ImGui's example_win32_directx10.

#include <d3d10.h>

#include "demo_common.h"
#include "demo_win32.h"
#include "foxyhud/renderers/dx10.h"

namespace {

ID3D10Device*           g_device = nullptr;
IDXGISwapChain*         g_swap_chain = nullptr;
ID3D10RenderTargetView* g_rtv = nullptr;
bool                    g_occluded = false;
UINT                    g_resize_w = 0, g_resize_h = 0;

void CreateRenderTarget() {
    ID3D10Texture2D* back_buffer = nullptr;
    g_swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer));
    g_device->CreateRenderTargetView(back_buffer, nullptr, &g_rtv);
    back_buffer->Release();
}

void CleanupRenderTarget() {
    if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; }
}

bool CreateDevice(HWND hwnd) {
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    HRESULT res = D3D10CreateDeviceAndSwapChain(nullptr, D3D10_DRIVER_TYPE_HARDWARE, nullptr, 0, D3D10_SDK_VERSION, &sd,
                                                &g_swap_chain, &g_device);
    if (res == DXGI_ERROR_UNSUPPORTED)  // fall back to the WARP software rasterizer
        res = D3D10CreateDeviceAndSwapChain(nullptr, D3D10_DRIVER_TYPE_WARP, nullptr, 0, D3D10_SDK_VERSION, &sd,
                                            &g_swap_chain, &g_device);
    if (res != S_OK)
        return false;
    CreateRenderTarget();
    return true;
}

void CleanupDevice() {
    CleanupRenderTarget();
    if (g_swap_chain) { g_swap_chain->Release(); g_swap_chain = nullptr; }
    if (g_device) { g_device->Release(); g_device = nullptr; }
}

LRESULT WINAPI WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (foxy::win32::HandleMessage(hwnd, msg, wparam, lparam))
        return true;
    if (msg == WM_SIZE) {
        if (wparam != SIZE_MINIMIZED) {
            g_resize_w = LOWORD(lparam);
            g_resize_h = HIWORD(lparam);
        }
        return 0;
    }
    return demo::DefaultWindowProc(hwnd, msg, wparam, lparam);
}

} // namespace

int main(int, char**) {
    demo::Window window;
    if (!demo::CreateAppWindow(window, L"FoxyHUD Menu - DirectX 10", WndProc) || !CreateDevice(window.hwnd)) {
        CleanupDevice();
        demo::DestroyAppWindow(window);
        return 1;
    }
    demo::ShowAppWindow(window);

    foxy::dx10::Init(window.hwnd, g_device, window.scale);
    ImGui::GetIO().IniFilename = "foxyhud_demo.ini";
    foxy::AdminPanel panel;

    const float clear[4] = {demo::kClearColor[0], demo::kClearColor[1], demo::kClearColor[2], 1.0f};
    while (demo::PumpMessages()) {
        if (g_occluded && g_swap_chain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) {
            ::Sleep(10);
            continue;
        }
        g_occluded = false;
        if (g_resize_w != 0 && g_resize_h != 0) {
            CleanupRenderTarget();
            g_swap_chain->ResizeBuffers(0, g_resize_w, g_resize_h, DXGI_FORMAT_UNKNOWN, 0);
            g_resize_w = g_resize_h = 0;
            CreateRenderTarget();
        }

        foxy::dx10::NewFrame();
        demo::DrawFrame(panel);

        g_device->OMSetRenderTargets(1, &g_rtv, nullptr);
        g_device->ClearRenderTargetView(g_rtv, clear);
        foxy::dx10::Render();
        g_occluded = g_swap_chain->Present(1, 0) == DXGI_STATUS_OCCLUDED;
    }

    foxy::dx10::Shutdown();
    ImGui::DestroyContext();
    CleanupDevice();
    demo::DestroyAppWindow(window);
    return 0;
}
