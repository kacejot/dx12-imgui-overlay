#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <dxgi1_2.h>

#include "hooking.h"

#include <imgui.h>
#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_win32.h>

#include <thread>
#include <chrono>
#include <fstream>
#include <format>
#include <filesystem>

using namespace std::chrono_literals;

// --------------------------------------------------------
// Лог
// --------------------------------------------------------

std::ofstream g_log("s2_overlay_log.txt", std::ios::out | std::ios::trunc);

#define LOG(fmt, ...)                                                    \
    do {                                                                 \
        if (g_log) {                                                     \
            auto _msg = std::format(fmt, ##__VA_ARGS__);                 \
            g_log << _msg << std::endl;                                  \
            g_log.flush();                                               \
        }                                                                \
    } while (0)

// --------------------------------------------------------
// Typedef'ы и глобалы
// --------------------------------------------------------


HMODULE g_dxgi_handle = nullptr;

enum function_id_t : uint64_t
{
    CREATE_DXGI_FACTORY = 0,
    CREATE_DXGI_FACTORY1 = 1,
    CREATE_DXGI_FACTORY2 = 2,
    IDXGI_FACTORY_CREATE_SWAP_CHAIN = 3, 
    IDXGI_FACTORY1_CREATE_SWAP_CHAIN = 4,
    IDXGI_FACTORY2_CREATE_SWAP_CHAIN = 5,
    IDXGI_FACTORY2_CREATE_SWAP_CHAIN_FOR_HWND = 6,
    IDXGI_FACTORY2_CREATE_SWAP_CHAIN_FOR_CORE_WINDOW = 7,
    IDXGI_FACTORY2_CREATE_SWAP_CHAIN_FOR_COMPOSITION = 8,
    DXGI_SWAP_CHAIN_PRESENT
};

bool is_inside_module(void* addr, HMODULE expectedModule)
{
    HMODULE actualModule = nullptr;
    GetModuleHandleExA(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCSTR>(addr),
        &actualModule
    );

    if (expectedModule && actualModule && expectedModule == actualModule)
        return true;

    // Если модули не совпадают, выводим отладку
    char filename[MAX_PATH]{};
    GetModuleFileNameA(actualModule, filename, MAX_PATH);

    LOG("Address expected to be in {:x} but found in {}", (uintptr_t)expectedModule, filename);
    return false;
}

class Main
{
public:

    Main() : m_hooking(0)
    {
    }

    hooking m_hooking;
};

Main* g_main;

bool check(void* addr, function_id id)
{
    if (!is_inside_module(addr, g_dxgi_handle))
    {
        LOG("{} is not inside dxgi.dll module - skipping", id);
        return false;
    }

    if (g_main->m_hooking.has_hook(id))
    {
        // LOG("{} already hooked", id);
        return false;
    }

    return true;
}

using PresentFn = HRESULT(__stdcall*)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
using CreateSwapChainFn = HRESULT(__stdcall*)(IDXGIFactory* pFactory, IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain);

static PresentFn          g_origPresent = nullptr;
static CreateSwapChainFn  g_origCreateSwapChain = nullptr;

static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_mainRTV = nullptr;

static bool g_imguiInitialized = false;
static bool g_overlayVisible = true;

using create_dxgi_factory_t = HRESULT(*)(const IID& riid, void** ppFactory);
using create_dxgi_factory1_t = HRESULT(*)(const IID& riid, void** ppFactory);
using create_dxgi_factory2_t = HRESULT(*)(uint32_t flags, const IID& riid, void** ppFactory);

using idxgi_factory_create_swap_chain_t = HRESULT(*)(uintptr_t self, IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain);

using idxgi_factory2_create_swap_chain_for_hwnd_t = HRESULT(*)(
    IUnknown* pDevice,
    HWND hWnd,
    const DXGI_SWAP_CHAIN_DESC1* pDesc,
    const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc,
    IDXGIOutput* pRestrictToOutput,
    IDXGISwapChain1** ppSwapChain
);

using idxgi_factory2_create_swap_chain_for_core_window_t = HRESULT(*)(
    IUnknown* pDevice,
    IUnknown* pWindow,
    const DXGI_SWAP_CHAIN_DESC1* pDesc,
    IDXGIOutput* pRestrictToOutput,
    IDXGISwapChain1** ppSwapChain
);

using idxgi_factory2_create_swap_chain_for_composition_t = HRESULT(*)(
    IUnknown* pDevice,
    const DXGI_SWAP_CHAIN_DESC1* pDesc,
    IDXGIOutput* pRestrictToOutput,
    IDXGISwapChain1** ppSwapChain
);

using dxgi_swap_chain_present_t = HRESULT(*)(uintptr_t self, UINT SyncInterval, UINT Flags);

HRESULT dxgi_swap_chain_present_hook(dxgi_swap_chain_present_t original, uintptr_t self, UINT SyncInterval, UINT Flags)
{
    LOG(" ---=== SUCCESS ===--- ");
    auto hr = original(self, SyncInterval, Flags);
    return hr;
}

HRESULT __fastcall idxgi_factory_create_swap_chain_hook(idxgi_factory_create_swap_chain_t original, uintptr_t self, IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain)
{
    LOG("idxgi_factory_create_swap_chain_hook called");
	auto hr = original(self, pDevice, pDesc, ppSwapChain);

    if (SUCCEEDED(hr) && ppSwapChain && *ppSwapChain)
    {
        IDXGISwapChain* sc = *ppSwapChain;
        void** vtable = *(void***)(sc);
        void* present_addr = vtable[8];

        if (!check(present_addr, DXGI_SWAP_CHAIN_PRESENT))
            return hr;

        g_main->m_hooking.add_hook<DXGI_SWAP_CHAIN_PRESENT>((uintptr_t)present_addr, dxgi_swap_chain_present_hook);
    };

    return hr;
}

HRESULT __fastcall idxgi_factory1_create_swap_chain_hook(idxgi_factory_create_swap_chain_t original, uintptr_t self, IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain)
{
    LOG("idxgi_factory1_create_swap_chain_hook called");
    auto hr = original(self, pDevice, pDesc, ppSwapChain);

    if (SUCCEEDED(hr) && ppSwapChain && *ppSwapChain)
    {
        IDXGISwapChain* sc = *ppSwapChain;
        void** vtable = *(void***)(sc);
        void* present_addr = vtable[8];

        if (!check(present_addr, DXGI_SWAP_CHAIN_PRESENT))
            return hr;

        g_main->m_hooking.add_hook<DXGI_SWAP_CHAIN_PRESENT>((uintptr_t)present_addr, dxgi_swap_chain_present_hook);
    }
    return hr;
}


HRESULT __fastcall idxgi_factory2_create_swap_chain_hook(
    idxgi_factory_create_swap_chain_t original,
    uintptr_t self, 
    IUnknown* pDevice,
    DXGI_SWAP_CHAIN_DESC* pDesc,
    IDXGISwapChain** ppSwapChain)
{
    LOG("idxgi_factory2_create_swap_chain_hook called");
    auto hr = original(self, pDevice, pDesc, ppSwapChain);

    if (SUCCEEDED(hr) && ppSwapChain && *ppSwapChain)
    {
        IDXGISwapChain* sc = *ppSwapChain;
        void** vtable = *(void***)(sc);
        void* present_addr = vtable[8];

        if (!check(present_addr, DXGI_SWAP_CHAIN_PRESENT))
            return hr;

        g_main->m_hooking.add_hook<DXGI_SWAP_CHAIN_PRESENT>((uintptr_t)present_addr, dxgi_swap_chain_present_hook);
    }
    return hr;
}

HRESULT __fastcall idxgi_factory2_create_swap_chain_for_hwnd(
    idxgi_factory2_create_swap_chain_for_hwnd_t original,
    IUnknown* pDevice,
    HWND hWnd,
    const DXGI_SWAP_CHAIN_DESC1* pDesc,
    const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc,
    IDXGIOutput* pRestrictToOutput,
    IDXGISwapChain1** ppSwapChain)
{
    LOG("idxgi_factory2_create_swap_chain_for_hwnd called");
    auto hr = original(pDevice, hWnd, pDesc, pFullscreenDesc, pRestrictToOutput, ppSwapChain);

    if (SUCCEEDED(hr) && ppSwapChain && *ppSwapChain)
    {
        IDXGISwapChain* sc = *ppSwapChain;
        void** vtable = *(void***)(sc);
        void* present_addr = vtable[8];

        if (!check(present_addr, DXGI_SWAP_CHAIN_PRESENT))
            return hr;

        g_main->m_hooking.add_hook<DXGI_SWAP_CHAIN_PRESENT>((uintptr_t)present_addr, dxgi_swap_chain_present_hook);
    }

    return hr;
}

HRESULT __fastcall idxgi_factory2_create_swap_chain_for_core_window(
    idxgi_factory2_create_swap_chain_for_core_window_t original,
    IUnknown* pDevice,
    IUnknown* pWindow,
    const DXGI_SWAP_CHAIN_DESC1* pDesc,
    IDXGIOutput* pRestrictToOutput,
    IDXGISwapChain1** ppSwapChain)
{
    LOG("idxgi_factory2_create_swap_chain_for_core_window called");
    auto hr = original(pDevice, pWindow, pDesc, pRestrictToOutput, ppSwapChain);
    
    if (SUCCEEDED(hr) && ppSwapChain && *ppSwapChain)
    {
        IDXGISwapChain* sc = *ppSwapChain;
        void** vtable = *(void***)(sc);
        void* present_addr = vtable[8];

        if (!check(present_addr, DXGI_SWAP_CHAIN_PRESENT))
            return hr;

        g_main->m_hooking.add_hook<DXGI_SWAP_CHAIN_PRESENT>((uintptr_t)present_addr, dxgi_swap_chain_present_hook);
    }

    return hr;
}

HRESULT __fastcall idxgi_factory2_create_swap_chain_for_composition(
    idxgi_factory2_create_swap_chain_for_composition_t original,
    IUnknown* pDevice,
    const DXGI_SWAP_CHAIN_DESC1* pDesc,
    IDXGIOutput* pRestrictToOutput,
    IDXGISwapChain1** ppSwapChain)
{
    LOG("idxgi_factory2_create_swap_chain_for_composition called");
    auto hr = original(pDevice, pDesc, pRestrictToOutput, ppSwapChain);
    
    if (SUCCEEDED(hr) && ppSwapChain && *ppSwapChain)
    {
        IDXGISwapChain* sc = *ppSwapChain;
        void** vtable = *(void***)(sc);
        void* present_addr = vtable[8];

        if (!check(present_addr, DXGI_SWAP_CHAIN_PRESENT))
            return hr;

        g_main->m_hooking.add_hook<DXGI_SWAP_CHAIN_PRESENT>((uintptr_t)present_addr, dxgi_swap_chain_present_hook);
    }

    return hr;
}

bool hook_dxgi_factory2_create_swap_chain(IUnknown* factory)
{
    if (!factory)
        return false;

    IDXGIFactory2* f2 = nullptr;
    if (FAILED(factory->QueryInterface(__uuidof(IDXGIFactory2), (void**)&f2)))
    {
        LOG("Failed to query interface");
        return false;
    }

    void** vtable = *(void***)(f2);

    // IDXGIFactory2::CreateSwapChain
    void* target = vtable[10];
    if (check(target, IDXGI_FACTORY2_CREATE_SWAP_CHAIN))
        g_main->m_hooking.add_hook<IDXGI_FACTORY2_CREATE_SWAP_CHAIN>((uintptr_t)target, idxgi_factory2_create_swap_chain_hook);

    // IDXGIFactory2::CreateSwapChainForHwnd
    target = vtable[15];
    if (check(target, IDXGI_FACTORY2_CREATE_SWAP_CHAIN_FOR_HWND))
        g_main->m_hooking.add_hook<IDXGI_FACTORY2_CREATE_SWAP_CHAIN_FOR_HWND>((uintptr_t)target, idxgi_factory2_create_swap_chain_for_hwnd);

    // IDXGIFactory2::CreateSwapChainForCoreWindow
    target = vtable[16];
    if (check(target, IDXGI_FACTORY2_CREATE_SWAP_CHAIN_FOR_CORE_WINDOW))
        g_main->m_hooking.add_hook<IDXGI_FACTORY2_CREATE_SWAP_CHAIN_FOR_CORE_WINDOW>((uintptr_t)target, idxgi_factory2_create_swap_chain_for_core_window);

    // IDXGIFactory2::CreateSwapChainForComposition
    target = vtable[24];
    if (check(target, IDXGI_FACTORY2_CREATE_SWAP_CHAIN_FOR_COMPOSITION))
        g_main->m_hooking.add_hook<IDXGI_FACTORY2_CREATE_SWAP_CHAIN_FOR_COMPOSITION>((uintptr_t)target, idxgi_factory2_create_swap_chain_for_composition);

    f2->Release();
    return true;
}

HRESULT create_dxgi_factory_hook(create_dxgi_factory_t original, const IID& riid, void** ppFactory)
{
    HRESULT hr = original(riid, ppFactory);
    if (FAILED(hr) || ppFactory == nullptr || *ppFactory == nullptr)
        return hr;

    void* factory = *ppFactory;
    void** vtable = *(void***)(factory);
    auto create_swap_chain = (idxgi_factory_create_swap_chain_t)vtable[10];

    if (!check(create_swap_chain, IDXGI_FACTORY_CREATE_SWAP_CHAIN))
        return hr;

    g_main->m_hooking.add_hook<IDXGI_FACTORY_CREATE_SWAP_CHAIN>((uintptr_t)create_swap_chain, idxgi_factory_create_swap_chain_hook);
  
    hook_dxgi_factory2_create_swap_chain((IUnknown*)*ppFactory);
    return hr;
}

HRESULT create_dxgi_factory1_hook(create_dxgi_factory_t original, const IID& riid, void** ppFactory)
{
    HRESULT hr = original(riid, ppFactory);
    if (FAILED(hr) || ppFactory == nullptr || *ppFactory == nullptr)
        return hr;

    void* factory = *ppFactory;
    void** vtable = *(void***)(factory);
    auto create_swap_chain = (idxgi_factory_create_swap_chain_t)vtable[10];

    if (!check(create_swap_chain, IDXGI_FACTORY1_CREATE_SWAP_CHAIN))
        return hr;

    g_main->m_hooking.add_hook<IDXGI_FACTORY1_CREATE_SWAP_CHAIN>((uintptr_t)create_swap_chain, idxgi_factory1_create_swap_chain_hook);

    hook_dxgi_factory2_create_swap_chain((IUnknown*)*ppFactory);
    return hr;
}

HRESULT create_dxgi_factory2_hook(create_dxgi_factory2_t original, uint32_t flags, const IID& riid, void** ppFactory)
{
    HRESULT hr = original(flags, riid, ppFactory);

    if (FAILED(hr) || !ppFactory || !*ppFactory)
        return hr;

    if (!hook_dxgi_factory2_create_swap_chain((IUnknown*)*ppFactory))
    {
        return hr;
    }

    return hr;
}

void init()
{
    auto& h = g_main->m_hooking;
    h.init();

    char buf[MAX_PATH];
    if (GetSystemDirectoryA(&buf[0], ARRAYSIZE(buf)) == 0) {
        return;
    }

    auto system_dxgi_path = std::filesystem::path{ buf } / "dxgi.dll";
    g_dxgi_handle = GetModuleHandleA(system_dxgi_path.generic_string().c_str());

	if (NULL == g_dxgi_handle) {
        LOG("Failed to get dxgi.dll handle!");
        return;
    }
    LOG("dxgi.dll handle is {:x}", (uintptr_t)g_dxgi_handle);

    auto p_create_dxgi_factory = (create_dxgi_factory_t)GetProcAddress(g_dxgi_handle, "CreateDXGIFactory");

	if (NULL == p_create_dxgi_factory) {
        LOG("Failed to get CreateDXGIFactory address!");
        return;
    }

    h.add_hook<CREATE_DXGI_FACTORY>((uintptr_t)p_create_dxgi_factory, create_dxgi_factory_hook);

	auto p_create_dxgi_factory1 = (create_dxgi_factory1_t)GetProcAddress(g_dxgi_handle, "CreateDXGIFactory1");
    if (NULL == p_create_dxgi_factory1) {
        LOG("Failed to get CreateDXGIFactory1 address!");
        return;
	}

    h.add_hook<CREATE_DXGI_FACTORY1>((uintptr_t)p_create_dxgi_factory1, create_dxgi_factory1_hook);


	auto p_create_dxgi_factory2 = (create_dxgi_factory2_t)GetProcAddress(g_dxgi_handle, "CreateDXGIFactory2");
    if (NULL == p_create_dxgi_factory2) {
        LOG("Failed to get CreateDXGIFactory2 address!");
        return;
	}

    h.add_hook<CREATE_DXGI_FACTORY2>((uintptr_t)p_create_dxgi_factory2, create_dxgi_factory2_hook);

}

extern "C" void init_plugin(HMODULE h_module)
{
    g_main = new Main;
    init();
}

extern "C" void deinit_plugin()
{
    delete g_main;
}
