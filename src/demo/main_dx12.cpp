// DirectX 12 demo: a window with the menu and nothing else. Press INSERT to toggle the menu.
// Device setup follows Dear ImGui's example_win32_directx12.

#include <d3d12.h>
#include <dxgi1_4.h>

#include "demo_common.h"
#include "demo_win32.h"
#include "foxyhud/renderers/dx12.h"

namespace {

constexpr int kFramesInFlight = 2;
constexpr int kBackBuffers = 2;
constexpr DXGI_FORMAT kBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

struct FrameContext {
    ID3D12CommandAllocator* allocator = nullptr;
    UINT64                  fence_value = 0;
};

FrameContext                g_frames[kFramesInFlight];
UINT                        g_frame_index = 0;
ID3D12Device*               g_device = nullptr;
ID3D12DescriptorHeap*       g_rtv_heap = nullptr;
ID3D12CommandQueue*         g_queue = nullptr;
ID3D12GraphicsCommandList*  g_cmd = nullptr;
ID3D12Fence*                g_fence = nullptr;
HANDLE                      g_fence_event = nullptr;
UINT64                      g_fence_last = 0;
IDXGISwapChain3*            g_swap_chain = nullptr;
HANDLE                      g_swap_chain_waitable = nullptr;
bool                        g_occluded = false;
ID3D12Resource*             g_back_buffers[kBackBuffers] = {};
D3D12_CPU_DESCRIPTOR_HANDLE g_rtv[kBackBuffers] = {};

void WaitForGpu() {
    g_queue->Signal(g_fence, ++g_fence_last);
    g_fence->SetEventOnCompletion(g_fence_last, g_fence_event);
    ::WaitForSingleObject(g_fence_event, INFINITE);
}

void CreateRenderTargets() {
    for (int i = 0; i < kBackBuffers; ++i) {
        g_swap_chain->GetBuffer(i, IID_PPV_ARGS(&g_back_buffers[i]));
        g_device->CreateRenderTargetView(g_back_buffers[i], nullptr, g_rtv[i]);
    }
}

void CleanupRenderTargets() {
    WaitForGpu();
    for (ID3D12Resource*& rt : g_back_buffers)
        if (rt) { rt->Release(); rt = nullptr; }
}

bool CreateDevice(HWND hwnd) {
    if (D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&g_device)) != S_OK)
        return false;

    D3D12_DESCRIPTOR_HEAP_DESC rtv_desc = {};
    rtv_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtv_desc.NumDescriptors = kBackBuffers;
    rtv_desc.NodeMask = 1;
    if (g_device->CreateDescriptorHeap(&rtv_desc, IID_PPV_ARGS(&g_rtv_heap)) != S_OK)
        return false;
    const SIZE_T rtv_size = g_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    D3D12_CPU_DESCRIPTOR_HANDLE rtv = g_rtv_heap->GetCPUDescriptorHandleForHeapStart();
    for (int i = 0; i < kBackBuffers; ++i, rtv.ptr += rtv_size)
        g_rtv[i] = rtv;

    D3D12_COMMAND_QUEUE_DESC queue_desc = {};
    queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queue_desc.NodeMask = 1;
    if (g_device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&g_queue)) != S_OK)
        return false;
    for (FrameContext& f : g_frames)
        if (g_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&f.allocator)) != S_OK)
            return false;
    if (g_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_frames[0].allocator, nullptr, IID_PPV_ARGS(&g_cmd)) != S_OK ||
        g_cmd->Close() != S_OK)
        return false;
    if (g_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_fence)) != S_OK)
        return false;
    if ((g_fence_event = ::CreateEventW(nullptr, FALSE, FALSE, nullptr)) == nullptr)
        return false;

    DXGI_SWAP_CHAIN_DESC1 sd = {};
    sd.BufferCount = kBackBuffers;
    sd.Format = kBackBufferFormat;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.SampleDesc.Count = 1;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    sd.Scaling = DXGI_SCALING_STRETCH;

    IDXGIFactory4* factory = nullptr;
    IDXGISwapChain1* swap_chain1 = nullptr;
    if (CreateDXGIFactory1(IID_PPV_ARGS(&factory)) != S_OK)
        return false;
    const bool ok = factory->CreateSwapChainForHwnd(g_queue, hwnd, &sd, nullptr, nullptr, &swap_chain1) == S_OK &&
                    swap_chain1->QueryInterface(IID_PPV_ARGS(&g_swap_chain)) == S_OK;
    if (swap_chain1)
        swap_chain1->Release();
    factory->Release();
    if (!ok)
        return false;
    g_swap_chain->SetMaximumFrameLatency(kBackBuffers);
    g_swap_chain_waitable = g_swap_chain->GetFrameLatencyWaitableObject();

    CreateRenderTargets();
    return true;
}

void CleanupDevice() {
    if (g_queue && g_fence && g_fence_event)
        CleanupRenderTargets();
    if (g_swap_chain) { g_swap_chain->Release(); g_swap_chain = nullptr; }
    if (g_swap_chain_waitable) { ::CloseHandle(g_swap_chain_waitable); g_swap_chain_waitable = nullptr; }
    for (FrameContext& f : g_frames)
        if (f.allocator) { f.allocator->Release(); f.allocator = nullptr; }
    if (g_cmd) { g_cmd->Release(); g_cmd = nullptr; }
    if (g_queue) { g_queue->Release(); g_queue = nullptr; }
    if (g_rtv_heap) { g_rtv_heap->Release(); g_rtv_heap = nullptr; }
    if (g_fence) { g_fence->Release(); g_fence = nullptr; }
    if (g_fence_event) { ::CloseHandle(g_fence_event); g_fence_event = nullptr; }
    if (g_device) { g_device->Release(); g_device = nullptr; }
}

FrameContext* WaitForNextFrame() {
    FrameContext* frame = &g_frames[g_frame_index % kFramesInFlight];
    if (g_fence->GetCompletedValue() < frame->fence_value) {
        g_fence->SetEventOnCompletion(frame->fence_value, g_fence_event);
        HANDLE waitables[] = {g_swap_chain_waitable, g_fence_event};
        ::WaitForMultipleObjects(2, waitables, TRUE, INFINITE);
    } else {
        ::WaitForSingleObject(g_swap_chain_waitable, INFINITE);
    }
    return frame;
}

LRESULT WINAPI WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (foxy::win32::HandleMessage(hwnd, msg, wparam, lparam))
        return true;
    if (msg == WM_SIZE) {
        if (g_device && wparam != SIZE_MINIMIZED) {
            CleanupRenderTargets();
            DXGI_SWAP_CHAIN_DESC1 desc = {};
            g_swap_chain->GetDesc1(&desc);
            g_swap_chain->ResizeBuffers(0, LOWORD(lparam), HIWORD(lparam), desc.Format, desc.Flags);
            CreateRenderTargets();
        }
        return 0;
    }
    return demo::DefaultWindowProc(hwnd, msg, wparam, lparam);
}

void Transition(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) {
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = resource;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = before;
    barrier.Transition.StateAfter = after;
    g_cmd->ResourceBarrier(1, &barrier);
}

} // namespace

int main(int, char**) {
    demo::Window window;
    if (!demo::CreateAppWindow(window, L"FoxyHUD Menu - DirectX 12", WndProc) || !CreateDevice(window.hwnd)) {
        CleanupDevice();
        demo::DestroyAppWindow(window);
        return 1;
    }
    demo::ShowAppWindow(window);

    foxy::dx12::Init(window.hwnd, g_device, g_queue, kFramesInFlight, kBackBufferFormat, window.scale);
    ImGui::GetIO().IniFilename = "foxyhud_demo.ini";
    foxy::AdminPanel panel;

    const float clear[4] = {demo::kClearColor[0], demo::kClearColor[1], demo::kClearColor[2], 1.0f};
    while (demo::PumpMessages()) {
        if ((g_occluded && g_swap_chain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) || ::IsIconic(window.hwnd)) {
            ::Sleep(10);
            continue;
        }
        g_occluded = false;

        foxy::dx12::NewFrame();
        demo::DrawFrame(panel);

        FrameContext* frame = WaitForNextFrame();
        const UINT bb = g_swap_chain->GetCurrentBackBufferIndex();
        frame->allocator->Reset();
        g_cmd->Reset(frame->allocator, nullptr);
        Transition(g_back_buffers[bb], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        g_cmd->ClearRenderTargetView(g_rtv[bb], clear, 0, nullptr);
        g_cmd->OMSetRenderTargets(1, &g_rtv[bb], FALSE, nullptr);
        foxy::dx12::Render(g_cmd);
        Transition(g_back_buffers[bb], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        g_cmd->Close();
        g_queue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList* const*>(&g_cmd));
        g_queue->Signal(g_fence, ++g_fence_last);
        frame->fence_value = g_fence_last;

        g_occluded = g_swap_chain->Present(1, 0) == DXGI_STATUS_OCCLUDED;
        ++g_frame_index;
    }

    WaitForGpu();
    foxy::dx12::Shutdown();
    ImGui::DestroyContext();
    CleanupDevice();
    demo::DestroyAppWindow(window);
    return 0;
}
