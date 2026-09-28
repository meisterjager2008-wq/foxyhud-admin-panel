#include "foxyhud/renderers/dx12.h"

#include "imgui.h"
#include "imgui_impl_dx12.h"

namespace foxy::dx12 {
namespace {

// Shader-visible heap for the menu's textures (font atlas etc.) with a simple
// free-list allocator, as the ImGui DX12 backend asks the app for descriptors.
constexpr UINT kSrvHeapSize = 64;

struct SrvHeap {
    ID3D12DescriptorHeap*       heap = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE cpu_start = {};
    D3D12_GPU_DESCRIPTOR_HANDLE gpu_start = {};
    UINT                        increment = 0;
    ImVector<int>               free_indices;

    bool Create(ID3D12Device* device) {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = kSrvHeapSize;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        if (device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap)) != S_OK)
            return false;
        cpu_start = heap->GetCPUDescriptorHandleForHeapStart();
        gpu_start = heap->GetGPUDescriptorHandleForHeapStart();
        increment = device->GetDescriptorHandleIncrementSize(desc.Type);
        free_indices.clear();
        for (int n = kSrvHeapSize; n > 0; --n)
            free_indices.push_back(n - 1);
        return true;
    }

    void Destroy() {
        if (heap)
            heap->Release();
        heap = nullptr;
        free_indices.clear();
    }

    void Alloc(D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu) {
        IM_ASSERT(free_indices.Size > 0 && "foxy::dx12: SRV heap is full");
        const int idx = free_indices.back();
        free_indices.pop_back();
        out_cpu->ptr = cpu_start.ptr + static_cast<SIZE_T>(idx) * increment;
        out_gpu->ptr = gpu_start.ptr + static_cast<UINT64>(idx) * increment;
    }

    void Free(D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE) {
        free_indices.push_back(static_cast<int>((cpu.ptr - cpu_start.ptr) / increment));
    }
};

SrvHeap g_srv;

} // namespace

bool Init(HWND hwnd, ID3D12Device* device, ID3D12CommandQueue* queue, int frames_in_flight, DXGI_FORMAT rtv_format,
          float ui_scale) {
    if (!win32::Init(hwnd, ui_scale) || !g_srv.Create(device))
        return false;

    ImGui_ImplDX12_InitInfo info;
    info.Device = device;
    info.CommandQueue = queue;
    info.NumFramesInFlight = frames_in_flight;
    info.RTVFormat = rtv_format;
    info.DSVFormat = DXGI_FORMAT_UNKNOWN;
    info.SrvDescriptorHeap = g_srv.heap;
    info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* cpu,
                                   D3D12_GPU_DESCRIPTOR_HANDLE* gpu) { g_srv.Alloc(cpu, gpu); };
    info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu,
                                  D3D12_GPU_DESCRIPTOR_HANDLE gpu) { g_srv.Free(cpu, gpu); };
    return ImGui_ImplDX12_Init(&info);
}

void NewFrame() {
    ImGui_ImplDX12_NewFrame();
    win32::NewFrame();
    ImGui::NewFrame();
}

void Render(ID3D12GraphicsCommandList* command_list) {
    ImGui::Render();
    command_list->SetDescriptorHeaps(1, &g_srv.heap);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), command_list);
}

void Shutdown() {
    ImGui_ImplDX12_Shutdown();
    win32::Shutdown();
    g_srv.Destroy();
}

} // namespace foxy::dx12
