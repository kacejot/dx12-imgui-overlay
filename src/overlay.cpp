#include "overlay.h"
#include "log.h"

#include <MinHook.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <d3d12.h>
#include <imgui.h>
#include <backends/imgui_impl_dx12.h>
#include <backends/imgui_impl_win32.h>
#include <filesystem>
#include <array>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace overlay {

namespace {

struct frame_context {
    ID3D12CommandAllocator*     command_allocator = nullptr;
    ID3D12Resource*             back_buffer = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = {};
    UINT64                      fence_value = 0;
};

struct state {
    ID3D12Device*               device = nullptr;
    ID3D12CommandQueue*         command_queue = nullptr;
    ID3D12DescriptorHeap*       rtv_heap = nullptr;
    ID3D12DescriptorHeap*       srv_heap = nullptr;
    ID3D12GraphicsCommandList*  command_list = nullptr;
    ID3D12Fence*                fence = nullptr;
    HANDLE                      fence_event = nullptr;
    UINT64                      fence_value = 0;
    frame_context*              frame_ctx = nullptr;
    HWND                        hwnd = nullptr;
    UINT                        buffer_count = 0;
    DXGI_FORMAT                 format = DXGI_FORMAT_R8G8B8A8_UNORM;
    bool                        initialized = false;
    bool                        show_ui = true;
};

using create_dxgi_factory_t         = HRESULT(WINAPI*)(REFIID, void**);
using create_dxgi_factory1_t        = HRESULT(WINAPI*)(REFIID, void**);
using create_dxgi_factory2_t        = HRESULT(WINAPI*)(UINT, REFIID, void**);
using create_swap_chain_t           = HRESULT(STDMETHODCALLTYPE*)(IDXGIFactory*, IUnknown*, DXGI_SWAP_CHAIN_DESC*, IDXGISwapChain**);
using create_swap_chain_for_hwnd_t  = HRESULT(STDMETHODCALLTYPE*)(IDXGIFactory2*, IUnknown*, HWND, const DXGI_SWAP_CHAIN_DESC1*, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC*, IDXGIOutput*, IDXGISwapChain1**);
using present_t                     = HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain3*, UINT, UINT);
using resize_buffers_t              = HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);

state             g_state;
render_callback_t g_render_callback = nullptr;
HMODULE           g_dxgi_handle = nullptr;
UINT              g_toggle_key = VK_OEM_MINUS;

create_dxgi_factory_t         g_orig_create_dxgi_factory = nullptr;
create_dxgi_factory1_t        g_orig_create_dxgi_factory1 = nullptr;
create_dxgi_factory2_t        g_orig_create_dxgi_factory2 = nullptr;
create_swap_chain_t           g_orig_create_swap_chain = nullptr;
create_swap_chain_for_hwnd_t  g_orig_create_swap_chain_for_hwnd = nullptr;
present_t                     g_orig_present = nullptr;
resize_buffers_t              g_orig_resize_buffers = nullptr;
WNDPROC                       g_orig_wnd_proc = nullptr;

void* g_hooked_create_swap_chain = nullptr;
void* g_hooked_create_swap_chain_for_hwnd = nullptr;
void* g_hooked_present = nullptr;
void* g_hooked_resize_buffers = nullptr;

HMODULE get_system_dxgi() {
    std::array<char, MAX_PATH> buf{};
    if (GetSystemDirectoryA(buf.data(), (UINT)buf.size()) == 0)
        return nullptr;
    auto path = std::filesystem::path{buf.data()} / "dxgi.dll";
    return GetModuleHandleA(path.string().c_str());
}

bool is_inside_dxgi(void* addr) {
    HMODULE mod = nullptr;
    GetModuleHandleExA(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCSTR)addr, &mod);
    return mod && mod == g_dxgi_handle;
}

void hook_vtable_method(void* target, void* detour, void** original, void** tracked) {
    if (!target || !is_inside_dxgi(target)) return;
    if (*tracked == target) return;
    
    if (MH_CreateHook(target, detour, original) == MH_OK) {
        MH_EnableHook(target);
        *tracked = target;
        LOG(" --- Hooked vtable method at {:x}", (uintptr_t)target);
    }
}

void wait_for_gpu() {
    if (!g_state.fence || !g_state.fence_event || !g_state.command_queue)
        return;
    g_state.fence_value++;
    g_state.command_queue->Signal(g_state.fence, g_state.fence_value);
    if (g_state.fence->GetCompletedValue() < g_state.fence_value) {
        g_state.fence->SetEventOnCompletion(g_state.fence_value, g_state.fence_event);
        WaitForSingleObject(g_state.fence_event, INFINITE);
    }
}

void release_frame_resources() {
    if (!g_state.frame_ctx)
        return;
    for (UINT i = 0; i < g_state.buffer_count; i++) {
        if (g_state.frame_ctx[i].back_buffer) {
            g_state.frame_ctx[i].back_buffer->Release();
            g_state.frame_ctx[i].back_buffer = nullptr;
        }
        g_state.frame_ctx[i].fence_value = 0;
    }
}

void recreate_frame_resources(IDXGISwapChain3* swap_chain) {
    if (!g_state.rtv_heap || !g_state.device || !g_state.frame_ctx)
        return;

    UINT rtv_size = g_state.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = g_state.rtv_heap->GetCPUDescriptorHandleForHeapStart();

    for (UINT i = 0; i < g_state.buffer_count; i++) {
        g_state.frame_ctx[i].rtv_handle = rtv_handle;
        ID3D12Resource* buf = nullptr;
        swap_chain->GetBuffer(i, IID_PPV_ARGS(&buf));
        g_state.device->CreateRenderTargetView(buf, nullptr, rtv_handle);
        g_state.frame_ctx[i].back_buffer = buf;
        rtv_handle.ptr += rtv_size;
    }
}

void cleanup_partial_init() {
    if (g_state.frame_ctx) {
        for (UINT i = 0; i < g_state.buffer_count; i++) {
            if (g_state.frame_ctx[i].command_allocator)
                g_state.frame_ctx[i].command_allocator->Release();
            if (g_state.frame_ctx[i].back_buffer)
                g_state.frame_ctx[i].back_buffer->Release();
        }
        delete[] g_state.frame_ctx;
        g_state.frame_ctx = nullptr;
    }
    if (g_state.fence_event) { CloseHandle(g_state.fence_event); g_state.fence_event = nullptr; }
    if (g_state.fence) { g_state.fence->Release(); g_state.fence = nullptr; }
    if (g_state.command_list) { g_state.command_list->Release(); g_state.command_list = nullptr; }
    if (g_state.srv_heap) { g_state.srv_heap->Release(); g_state.srv_heap = nullptr; }
    if (g_state.rtv_heap) { g_state.rtv_heap->Release(); g_state.rtv_heap = nullptr; }
    if (g_state.device) { g_state.device->Release(); g_state.device = nullptr; }
}

LRESULT CALLBACK hook_wnd_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_KEYUP && wParam == g_toggle_key) {
        g_state.show_ui = !g_state.show_ui;
        return TRUE;
    }
    if (g_state.show_ui && ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return TRUE;
    return CallWindowProcW(g_orig_wnd_proc, hWnd, msg, wParam, lParam);
}

HRESULT STDMETHODCALLTYPE hook_present(IDXGISwapChain3* self, UINT SyncInterval, UINT Flags) {
    if (!g_state.initialized && g_state.command_queue) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.MouseDrawCursor = true;

        LOG(" --- Initializing DX12 ImGui");

        DXGI_SWAP_CHAIN_DESC desc;
        if (FAILED(self->GetDevice(__uuidof(ID3D12Device), (void**)&g_state.device)) || !g_state.device) {
            LOG(" --- Failed to get D3D12 device");
            ImGui::DestroyContext();
            return g_orig_present(self, SyncInterval, Flags);
        }
        self->GetDesc(&desc);
        self->GetHwnd(&g_state.hwnd);
        
        g_state.buffer_count = desc.BufferCount;
        g_state.format = desc.BufferDesc.Format;
        g_state.frame_ctx = new frame_context[g_state.buffer_count]{};

        D3D12_DESCRIPTOR_HEAP_DESC srv_desc = {};
        srv_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srv_desc.NumDescriptors = g_state.buffer_count;
        srv_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        if (FAILED(g_state.device->CreateDescriptorHeap(&srv_desc, IID_PPV_ARGS(&g_state.srv_heap)))) {
            LOG(" --- Failed to create SRV heap");
            cleanup_partial_init();
            ImGui::DestroyContext();
            return g_orig_present(self, SyncInterval, Flags);
        }

        for (UINT i = 0; i < g_state.buffer_count; i++) {
            if (FAILED(g_state.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_state.frame_ctx[i].command_allocator)))) {
                LOG(" --- Failed to create command allocator {}", i);
                cleanup_partial_init();
                ImGui::DestroyContext();
                return g_orig_present(self, SyncInterval, Flags);
            }
        }

        if (FAILED(g_state.device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_state.frame_ctx[0].command_allocator, nullptr, IID_PPV_ARGS(&g_state.command_list)))) {
            LOG(" --- Failed to create command list");
            cleanup_partial_init();
            ImGui::DestroyContext();
            return g_orig_present(self, SyncInterval, Flags);
        }
        g_state.command_list->Close();

        if (FAILED(g_state.device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_state.fence)))) {
            LOG(" --- Failed to create fence");
            cleanup_partial_init();
            ImGui::DestroyContext();
            return g_orig_present(self, SyncInterval, Flags);
        }
        g_state.fence_event = CreateEventW(nullptr, FALSE, FALSE, nullptr);

        D3D12_DESCRIPTOR_HEAP_DESC rtv_desc = {};
        rtv_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtv_desc.NumDescriptors = g_state.buffer_count;
        rtv_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        rtv_desc.NodeMask = 1;
        if (FAILED(g_state.device->CreateDescriptorHeap(&rtv_desc, IID_PPV_ARGS(&g_state.rtv_heap)))) {
            LOG(" --- Failed to create RTV heap");
            cleanup_partial_init();
            ImGui::DestroyContext();
            return g_orig_present(self, SyncInterval, Flags);
        }

        recreate_frame_resources(self);

        ImGui_ImplWin32_Init(g_state.hwnd);
        ImGui_ImplDX12_Init(
            g_state.device,
            g_state.buffer_count,
            g_state.format,
            g_state.srv_heap,
            g_state.srv_heap->GetCPUDescriptorHandleForHeapStart(),
            g_state.srv_heap->GetGPUDescriptorHandleForHeapStart()
        );
        io.Fonts->Build();
        ImGui_ImplDX12_CreateDeviceObjects();

        g_orig_wnd_proc = (WNDPROC)SetWindowLongPtrW(g_state.hwnd, GWLP_WNDPROC, (LONG_PTR)hook_wnd_proc);

        g_state.initialized = true;
        LOG(" --- DX12 ImGui initialized successfully");
    }

    if (g_render_callback && g_state.command_queue && g_state.initialized && g_state.show_ui) {
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        g_render_callback(self, SyncInterval, Flags);

        UINT idx = self->GetCurrentBackBufferIndex();
        frame_context& ctx = g_state.frame_ctx[idx];

        if (ctx.fence_value != 0 && g_state.fence->GetCompletedValue() < ctx.fence_value) {
            g_state.fence->SetEventOnCompletion(ctx.fence_value, g_state.fence_event);
            WaitForSingleObject(g_state.fence_event, INFINITE);
        }

        ctx.command_allocator->Reset();

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = ctx.back_buffer;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

        g_state.command_list->Reset(ctx.command_allocator, nullptr);
        g_state.command_list->ResourceBarrier(1, &barrier);
        g_state.command_list->OMSetRenderTargets(1, &ctx.rtv_handle, FALSE, nullptr);
        g_state.command_list->SetDescriptorHeaps(1, &g_state.srv_heap);

        ImGui::Render();
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), g_state.command_list);

        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        g_state.command_list->ResourceBarrier(1, &barrier);
        g_state.command_list->Close();

        g_state.command_queue->ExecuteCommandLists(1, (ID3D12CommandList**)&g_state.command_list);

        ctx.fence_value = ++g_state.fence_value;
        g_state.command_queue->Signal(g_state.fence, ctx.fence_value);
    }

    return g_orig_present(self, SyncInterval, Flags);
}

HRESULT STDMETHODCALLTYPE hook_resize_buffers(
    IDXGISwapChain* self, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
    LOG(" --- ResizeBuffers {}x{}", Width, Height);

    if (g_state.initialized) {
        wait_for_gpu();
        release_frame_resources();
        ImGui_ImplDX12_InvalidateDeviceObjects();
    }

    HRESULT hr = g_orig_resize_buffers(self, BufferCount, Width, Height, NewFormat, SwapChainFlags);

    if (SUCCEEDED(hr) && g_state.initialized) {
        if (BufferCount != 0)
            g_state.buffer_count = BufferCount;
        if (NewFormat != DXGI_FORMAT_UNKNOWN)
            g_state.format = NewFormat;

        IDXGISwapChain3* sc3 = nullptr;
        if (SUCCEEDED(self->QueryInterface(__uuidof(IDXGISwapChain3), (void**)&sc3))) {
            recreate_frame_resources(sc3);
            sc3->Release();
        }
        ImGui_ImplDX12_CreateDeviceObjects();
    }

    return hr;
}

void hook_swap_chain_vtable(IDXGISwapChain* sc, IUnknown* pDevice) {
    g_state.command_queue = (ID3D12CommandQueue*)pDevice;
    void** vtable = *(void***)sc;
    hook_vtable_method(vtable[8], hook_present, (void**)&g_orig_present, &g_hooked_present);
    hook_vtable_method(vtable[13], hook_resize_buffers, (void**)&g_orig_resize_buffers, &g_hooked_resize_buffers);
}

HRESULT STDMETHODCALLTYPE hook_create_swap_chain(
    IDXGIFactory* self, IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain)
{
    LOG(" --- CreateSwapChain");
    HRESULT hr = g_orig_create_swap_chain(self, pDevice, pDesc, ppSwapChain);
    if (SUCCEEDED(hr) && ppSwapChain && *ppSwapChain)
        hook_swap_chain_vtable(*ppSwapChain, pDevice);
    return hr;
}

HRESULT STDMETHODCALLTYPE hook_create_swap_chain_for_hwnd(
    IDXGIFactory2* self, IUnknown* pDevice, HWND hWnd,
    const DXGI_SWAP_CHAIN_DESC1* pDesc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFsDesc,
    IDXGIOutput* pOutput, IDXGISwapChain1** ppSwapChain)
{
    LOG(" --- CreateSwapChainForHwnd");
    HRESULT hr = g_orig_create_swap_chain_for_hwnd(self, pDevice, hWnd, pDesc, pFsDesc, pOutput, ppSwapChain);
    if (SUCCEEDED(hr) && ppSwapChain && *ppSwapChain)
        hook_swap_chain_vtable((IDXGISwapChain*)*ppSwapChain, pDevice);
    return hr;
}

void hook_factory_vtable(void* factory) {
    void** vtable = *(void***)factory;
    hook_vtable_method(vtable[10], hook_create_swap_chain, (void**)&g_orig_create_swap_chain, &g_hooked_create_swap_chain);

    IDXGIFactory2* f2 = nullptr;
    if (SUCCEEDED(((IUnknown*)factory)->QueryInterface(__uuidof(IDXGIFactory2), (void**)&f2))) {
        void** vtable2 = *(void***)f2;
        hook_vtable_method(vtable2[15], hook_create_swap_chain_for_hwnd, (void**)&g_orig_create_swap_chain_for_hwnd, &g_hooked_create_swap_chain_for_hwnd);
        f2->Release();
    }
}

HRESULT WINAPI hook_create_dxgi_factory(REFIID riid, void** ppFactory) {
    LOG(" --- CreateDXGIFactory");
    HRESULT hr = g_orig_create_dxgi_factory(riid, ppFactory);
    if (SUCCEEDED(hr) && ppFactory && *ppFactory)
        hook_factory_vtable(*ppFactory);
    return hr;
}

HRESULT WINAPI hook_create_dxgi_factory1(REFIID riid, void** ppFactory) {
    LOG(" --- CreateDXGIFactory1");
    HRESULT hr = g_orig_create_dxgi_factory1(riid, ppFactory);
    if (SUCCEEDED(hr) && ppFactory && *ppFactory)
        hook_factory_vtable(*ppFactory);
    return hr;
}

HRESULT WINAPI hook_create_dxgi_factory2(UINT flags, REFIID riid, void** ppFactory) {
    LOG(" --- CreateDXGIFactory2");
    HRESULT hr = g_orig_create_dxgi_factory2(flags, riid, ppFactory);
    if (SUCCEEDED(hr) && ppFactory && *ppFactory)
        hook_factory_vtable(*ppFactory);
    return hr;
}

} // anonymous namespace

void init() {
    LOG("Overlay init");

    g_dxgi_handle = get_system_dxgi();
    if (!g_dxgi_handle) {
        LOG("Failed to get dxgi.dll handle");
        return;
    }

    MH_Initialize();

    MH_CreateHookApi(L"dxgi.dll", "CreateDXGIFactory", hook_create_dxgi_factory, (void**)&g_orig_create_dxgi_factory);
    MH_CreateHookApi(L"dxgi.dll", "CreateDXGIFactory1", hook_create_dxgi_factory1, (void**)&g_orig_create_dxgi_factory1);
    MH_CreateHookApi(L"dxgi.dll", "CreateDXGIFactory2", hook_create_dxgi_factory2, (void**)&g_orig_create_dxgi_factory2);

    MH_EnableHook(MH_ALL_HOOKS);

    LOG("Overlay hooks installed");
}

void deinit() {
    LOG("Overlay shutdown");

    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();

    if (g_state.initialized) {
        wait_for_gpu();

        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        if (g_orig_wnd_proc && g_state.hwnd)
            SetWindowLongPtrW(g_state.hwnd, GWLP_WNDPROC, (LONG_PTR)g_orig_wnd_proc);

        if (g_state.frame_ctx) {
            for (UINT i = 0; i < g_state.buffer_count; i++) {
                if (g_state.frame_ctx[i].command_allocator)
                    g_state.frame_ctx[i].command_allocator->Release();
                if (g_state.frame_ctx[i].back_buffer)
                    g_state.frame_ctx[i].back_buffer->Release();
            }
            delete[] g_state.frame_ctx;
        }
        if (g_state.fence_event) CloseHandle(g_state.fence_event);
        if (g_state.fence) g_state.fence->Release();
        if (g_state.command_list) g_state.command_list->Release();
        if (g_state.srv_heap) g_state.srv_heap->Release();
        if (g_state.rtv_heap) g_state.rtv_heap->Release();
        if (g_state.device) g_state.device->Release();
    }

    g_state = {};
}

void set_render_callback(render_callback_t render_callback) {
    g_render_callback = render_callback;
}

void set_toggle_key(UINT vk_code) {
    g_toggle_key = vk_code;
}

void enable_logging(const char* path) {
    ::enable_logging(path);
}

void disable_logging() {
    ::disable_logging();
}

}
