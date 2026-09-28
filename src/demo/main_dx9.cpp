// DirectX 9 demo: a window with the menu and nothing else. Press INSERT to toggle the menu.
// Device setup follows Dear ImGui's example_win32_directx9.

#include <d3d9.h>

#include "demo_common.h"
#include "demo_win32.h"
#include "foxyhud/renderers/dx9.h"

namespace {

IDirect3D9*           g_d3d = nullptr;
IDirect3DDevice9*     g_device = nullptr;
D3DPRESENT_PARAMETERS g_params = {};
bool                  g_device_lost = false;
UINT                  g_resize_w = 0, g_resize_h = 0;

bool CreateDevice(HWND hwnd) {
    if ((g_d3d = Direct3DCreate9(D3D_SDK_VERSION)) == nullptr)
        return false;
    ZeroMemory(&g_params, sizeof(g_params));
    g_params.Windowed = TRUE;
    g_params.SwapEffect = D3DSWAPEFFECT_DISCARD;
    g_params.BackBufferFormat = D3DFMT_UNKNOWN;
    g_params.EnableAutoDepthStencil = TRUE;
    g_params.AutoDepthStencilFormat = D3DFMT_D16;
    g_params.PresentationInterval = D3DPRESENT_INTERVAL_ONE;  // vsync
    return g_d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_params,
                               &g_device) >= 0;
}

void CleanupDevice() {
    if (g_device) { g_device->Release(); g_device = nullptr; }
    if (g_d3d) { g_d3d->Release(); g_d3d = nullptr; }
}

// The menu's GPU objects must be released before Reset() and recreated after.
void ResetDevice() {
    foxy::dx9::OnDeviceLost();
    if (g_device->Reset(&g_params) == D3DERR_INVALIDCALL)
        IM_ASSERT(0);
    foxy::dx9::OnDeviceReset();
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
    if (!demo::CreateAppWindow(window, L"FoxyHUD Menu - DirectX 9", WndProc) || !CreateDevice(window.hwnd)) {
        CleanupDevice();
        demo::DestroyAppWindow(window);
        return 1;
    }
    demo::ShowAppWindow(window);

    foxy::dx9::Init(window.hwnd, g_device, window.scale);
    ImGui::GetIO().IniFilename = "foxyhud_demo.ini";
    foxy::AdminPanel panel;

    const D3DCOLOR clear = D3DCOLOR_RGBA(static_cast<int>(demo::kClearColor[0] * 255), static_cast<int>(demo::kClearColor[1] * 255),
                                         static_cast<int>(demo::kClearColor[2] * 255), 255);
    while (demo::PumpMessages()) {
        if (g_device_lost) {
            const HRESULT hr = g_device->TestCooperativeLevel();
            if (hr == D3DERR_DEVICELOST) {
                ::Sleep(10);
                continue;
            }
            if (hr == D3DERR_DEVICENOTRESET)
                ResetDevice();
            g_device_lost = false;
        }
        if (g_resize_w != 0 && g_resize_h != 0) {
            g_params.BackBufferWidth = g_resize_w;
            g_params.BackBufferHeight = g_resize_h;
            g_resize_w = g_resize_h = 0;
            ResetDevice();
        }

        foxy::dx9::NewFrame();
        demo::DrawFrame(panel);

        g_device->SetRenderState(D3DRS_ZENABLE, FALSE);
        g_device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        g_device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        g_device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear, 1.0f, 0);
        if (g_device->BeginScene() >= 0) {
            foxy::dx9::Render();
            g_device->EndScene();
        } else {
            ImGui::EndFrame();
        }
        if (g_device->Present(nullptr, nullptr, nullptr, nullptr) == D3DERR_DEVICELOST)
            g_device_lost = true;
    }

    foxy::dx9::Shutdown();
    ImGui::DestroyContext();
    CleanupDevice();
    demo::DestroyAppWindow(window);
    return 0;
}
