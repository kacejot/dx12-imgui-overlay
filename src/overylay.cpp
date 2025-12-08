#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <dxgi1_2.h>

#include "hooking.h"
#include "dxgi_hooking.h"

#include <filesystem>

HMODULE g_dxgi_handle = nullptr;

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

    char filename[MAX_PATH]{};
    GetModuleFileNameA(actualModule, filename, MAX_PATH);

    LOG("Address expected to be in {:x} but found in {}", (uintptr_t)expectedModule, filename);
    return false;
}

class dxgi_hooking
{
public:

	dxgi_hooking(hooking& hk) : m_hooking(hk), dxgi_swap_chain_present(hk)
    {
    }

    hooking m_hooking;
	dxgi_swap_chain_present_hook dxgi_swap_chain_present;
};

struct master
{
    master() : hk(0), hk_dxgi(hk)
    {
    }

    static master& get()
    {
        static master instance;
        return instance;
    }

    hooking hk;
    dxgi_hooking hk_dxgi;
};

dxgi_hooking& get_hk()
{
    return master::get().hk_dxgi;
}

bool check(void* addr, function_id_t id)
{
    if (!is_inside_module(addr, g_dxgi_handle))
    {
        LOG("{} is not inside dxgi.dll module - skipping", (uint32_t)id);
        return false;
    }

    if (get_hk().m_hooking.has_hook(id))
    {
        // LOG("{} already hooked", id);
        return false;
    }

    return true;
}

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

        get_hk().m_hooking.add_hook<DXGI_SWAP_CHAIN_PRESENT>((uintptr_t)present_addr, get_hk().dxgi_swap_chain_present);
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

        get_hk().m_hooking.add_hook<DXGI_SWAP_CHAIN_PRESENT>((uintptr_t)present_addr, get_hk().dxgi_swap_chain_present);
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

        get_hk().m_hooking.add_hook<DXGI_SWAP_CHAIN_PRESENT>((uintptr_t)present_addr, get_hk().dxgi_swap_chain_present);
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

        get_hk().m_hooking.add_hook<DXGI_SWAP_CHAIN_PRESENT>((uintptr_t)present_addr, get_hk().dxgi_swap_chain_present);
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

        get_hk().m_hooking.add_hook<DXGI_SWAP_CHAIN_PRESENT>((uintptr_t)present_addr, get_hk().dxgi_swap_chain_present);
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

        get_hk().m_hooking.add_hook<DXGI_SWAP_CHAIN_PRESENT>((uintptr_t)present_addr, get_hk().dxgi_swap_chain_present);
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
        get_hk().m_hooking.add_hook<IDXGI_FACTORY2_CREATE_SWAP_CHAIN>((uintptr_t)target, idxgi_factory2_create_swap_chain_hook);

    // IDXGIFactory2::CreateSwapChainForHwnd
    target = vtable[15];
    if (check(target, IDXGI_FACTORY2_CREATE_SWAP_CHAIN_FOR_HWND))
        get_hk().m_hooking.add_hook<IDXGI_FACTORY2_CREATE_SWAP_CHAIN_FOR_HWND>((uintptr_t)target, idxgi_factory2_create_swap_chain_for_hwnd);

    // IDXGIFactory2::CreateSwapChainForCoreWindow
    target = vtable[16];
    if (check(target, IDXGI_FACTORY2_CREATE_SWAP_CHAIN_FOR_CORE_WINDOW))
        get_hk().m_hooking.add_hook<IDXGI_FACTORY2_CREATE_SWAP_CHAIN_FOR_CORE_WINDOW>((uintptr_t)target, idxgi_factory2_create_swap_chain_for_core_window);

    // IDXGIFactory2::CreateSwapChainForComposition
    target = vtable[24];
    if (check(target, IDXGI_FACTORY2_CREATE_SWAP_CHAIN_FOR_COMPOSITION))
        get_hk().m_hooking.add_hook<IDXGI_FACTORY2_CREATE_SWAP_CHAIN_FOR_COMPOSITION>((uintptr_t)target, idxgi_factory2_create_swap_chain_for_composition);

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

    get_hk().m_hooking.add_hook<IDXGI_FACTORY_CREATE_SWAP_CHAIN>((uintptr_t)create_swap_chain, idxgi_factory_create_swap_chain_hook);
  
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

    get_hk().m_hooking.add_hook<IDXGI_FACTORY1_CREATE_SWAP_CHAIN>((uintptr_t)create_swap_chain, idxgi_factory1_create_swap_chain_hook);

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
    auto& h = get_hk().m_hooking;
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
    init();
}

extern "C" void deinit_plugin()
{
}
