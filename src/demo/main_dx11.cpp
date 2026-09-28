// DirectX 11 demo: a window with the menu and nothing else. Press INSERT to toggle the menu.
// Device setup follows Dear ImGui's example_win32_directx11.

#include <d3d11.h>

#include "demo_common.h"
#include "demo_win32.h"
#include "foxyhud/renderers/dx11.h"

namespace {

ID3D11Device*           g_device = nullptr;
ID3D11DeviceContext*    g_context = nullptr;
IDXGISwapChain*         g_swap_chain = nullptr;
ID3D11RenderTargetView* g_rtv = nullptr;
bool                    g_occluded = false;
UINT                    g_resize_w = 0, g_resize_h = 0;

void CreateRenderTarget() {
    ID3D11Texture2D* back_buffer = nullptr;
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

    const D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
    D3D_FEATURE_LEVEL level;
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, levels, 2, D3D11_SDK_VERSION,
                                                &sd, &g_swap_chain, &g_device, &level, &g_context);
    if (res == DXGI_ERROR_UNSUPPORTED)  // fall back to the WARP software rasterizer
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, levels, 2, D3D11_SDK_VERSION, &sd,
                                            &g_swap_chain, &g_device, &level, &g_context);
    if (res != S_OK)
        return false;
    CreateRenderTarget();
    return true;
}

void CleanupDevice() {
    CleanupRenderTarget();
    if (g_swap_chain) { g_swap_chain->Release(); g_swap_chain = nullptr; }
    if (g_context) { g_context->Release(); g_context = nullptr; }
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
    if (!demo::CreateAppWindow(window, L"FoxyHUD Menu - DirectX 11", WndProc) || !CreateDevice(window.hwnd)) {
        CleanupDevice();
        demo::DestroyAppWindow(window);
        return 1;
    }
    demo::ShowAppWindow(window);

    foxy::dx11::Init(window.hwnd, g_device, g_context, window.scale);
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

        foxy::dx11::NewFrame();
        demo::DrawFrame(panel);

        g_context->OMSetRenderTargets(1, &g_rtv, nullptr);
        g_context->ClearRenderTargetView(g_rtv, clear);
        foxy::dx11::Render();
        g_occluded = g_swap_chain->Present(1, 0) == DXGI_STATUS_OCCLUDED;
    }

    foxy::dx11::Shutdown();
    ImGui::DestroyContext();
    CleanupDevice();
    demo::DestroyAppWindow(window);
    return 0;
}
