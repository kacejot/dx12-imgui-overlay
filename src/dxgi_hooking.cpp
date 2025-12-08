#include "dxgi_hooking.h"

#include <d3d11.h>
#include <dxgi.h>


HRESULT dxgi_swap_chain_present_hook::operator()(dxgi_swap_chain_present_t original, uintptr_t self, UINT SyncInterval, UINT Flags)
{
    LOG(" ---=== SUCCESS ===--- ");
    auto hr = original(self, SyncInterval, Flags);
    return hr;
}



//HRESULT create_dxgi_factory_hook::operator()(create_dxgi_factory_t original, const IID& riid, void** ppFactory)
//{
//    LOG("--- create factory");
//    HRESULT hr = original(riid, ppFactory);
//    if (FAILED(hr) || ppFactory == nullptr || *ppFactory == nullptr)
//        return hr;
//
//    void* factory = *ppFactory;
//    void** vtable = *(void***)(factory);
//    auto create_swap_chain = (idxgi_factory_create_swap_chain_t)vtable[10];
//
//    if (!m_master.check(create_swap_chain, IDXGI_FACTORY_CREATE_SWAP_CHAIN))
//        return hr;
//
//    m_hooking.add_hook<IDXGI_FACTORY_CREATE_SWAP_CHAIN>((uintptr_t)create_swap_chain, m_master.idxgi_factory_create_swap_chain);
//
//    m_master.create_f2_hooks((IUnknown*)*ppFactory);
//    return hr;
//}
//
//HRESULT create_dxgi_factory1_hook::operator()(create_dxgi_factory_t original, const IID& riid, void** ppFactory)
//{
//    LOG("--- create factory1");
//    HRESULT hr = original(riid, ppFactory);
//    if (FAILED(hr) || ppFactory == nullptr || *ppFactory == nullptr)
//        return hr;
//
//    void* factory = *ppFactory;
//    void** vtable = *(void***)(factory);
//    auto create_swap_chain = (idxgi_factory_create_swap_chain_t)vtable[10];
//
//    if (!m_master.check(create_swap_chain, IDXGI_FACTORY1_CREATE_SWAP_CHAIN))
//        return hr;
//
//    m_hooking.add_hook<IDXGI_FACTORY1_CREATE_SWAP_CHAIN>((uintptr_t)create_swap_chain, m_master.idxgi_factory1_create_swap_chain);
//
//    m_master.create_f2_hooks((IUnknown*)*ppFactory);
//    return hr;
//}
//
//
//HRESULT create_dxgi_factory2_hook::operator()(create_dxgi_factory2_t original, uint32_t flags, const IID& riid, void** ppFactory)
//{
//    LOG("--- create factory2");
//    HRESULT hr = original(flags, riid, ppFactory);
//
//    if (FAILED(hr) || !ppFactory || !*ppFactory)
//        return hr;
//
//    m_master.create_f2_hooks((IUnknown*)(*ppFactory));
//}
//
//HRESULT __fastcall idxgi_factory_create_swap_chain_hook::operator()(idxgi_factory_create_swap_chain_t original, uintptr_t self, IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain)
//{
//    LOG("--- factory -> create swap chain");
//    auto hr = original(self, pDevice, pDesc, ppSwapChain);
//
//    if (SUCCEEDED(hr) && ppSwapChain && *ppSwapChain)
//    {
//        IDXGISwapChain* sc = *ppSwapChain;
//        void** vtable = *(void***)(sc);
//        void* present_addr = vtable[8];
//
//        if (!m_master.check(present_addr, DXGI_SWAP_CHAIN_PRESENT))
//            return hr;
//
//        m_hooking.add_hook<DXGI_SWAP_CHAIN_PRESENT>((uintptr_t)present_addr, m_master.dxgi_swap_chain_present);
//    };
//
//    return hr;
//}
//
//HRESULT __fastcall idxgi_factory1_create_swap_chain_hook::operator()(idxgi_factory_create_swap_chain_t original, uintptr_t self, IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain)
//{
//    LOG("--- factory1 -> create swap chain");
//    auto hr = original(self, pDevice, pDesc, ppSwapChain);
//
//    if (SUCCEEDED(hr) && ppSwapChain && *ppSwapChain)
//    {
//        IDXGISwapChain* sc = *ppSwapChain;
//        void** vtable = *(void***)(sc);
//        void* present_addr = vtable[8];
//
//        if (!m_master.check(present_addr, DXGI_SWAP_CHAIN_PRESENT))
//            return hr;
//
//        m_hooking.add_hook<DXGI_SWAP_CHAIN_PRESENT>((uintptr_t)present_addr, m_master.dxgi_swap_chain_present);
//    }
//    return hr;
//}
//
//HRESULT __fastcall idxgi_factory2_create_swap_chain_hook::operator()(
//    idxgi_factory_create_swap_chain_t original,
//    uintptr_t self,
//    IUnknown* pDevice,
//    DXGI_SWAP_CHAIN_DESC* pDesc,
//    IDXGISwapChain** ppSwapChain)
//{
//    LOG("--- factory2 -> create swap chain");
//    auto hr = original(self, pDevice, pDesc, ppSwapChain);
//
//    if (SUCCEEDED(hr) && ppSwapChain && *ppSwapChain)
//    {
//        IDXGISwapChain* sc = *ppSwapChain;
//        void** vtable = *(void***)(sc);
//        void* present_addr = vtable[8];
//
//        if (!m_master.check(present_addr, DXGI_SWAP_CHAIN_PRESENT))
//            return hr;
//
//        m_hooking.add_hook<DXGI_SWAP_CHAIN_PRESENT>((uintptr_t)present_addr, m_master.dxgi_swap_chain_present);
//    }
//    return hr;
//}
//
//HRESULT __fastcall idxgi_factory2_create_swap_chain_for_hwnd_hook::operator()(
//    idxgi_factory2_create_swap_chain_for_hwnd_t original,
//    IUnknown* pDevice,
//    HWND hWnd,
//    const DXGI_SWAP_CHAIN_DESC1* pDesc,
//    const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc,
//    IDXGIOutput* pRestrictToOutput,
//    IDXGISwapChain1** ppSwapChain)
//{
//    LOG("--- factory2 -> create swap chain for hwnd");
//    auto hr = original(pDevice, hWnd, pDesc, pFullscreenDesc, pRestrictToOutput, ppSwapChain);
//
//    if (SUCCEEDED(hr) && ppSwapChain && *ppSwapChain)
//    {
//        IDXGISwapChain* sc = *ppSwapChain;
//        void** vtable = *(void***)(sc);
//        void* present_addr = vtable[8];
//
//		if (!m_master.try_add_hook<DXGI_SWAP_CHAIN_PRESENT>(present_addr, m_master.dxgi_swap_chain_present))
//            return hr;
//    }
//
//    return hr;
//}
//
//HRESULT __fastcall idxgi_factory2_create_swap_chain_for_core_window_hook::operator()(
//    idxgi_factory2_create_swap_chain_for_core_window_t original,
//    IUnknown* pDevice,
//    IUnknown* pWindow,
//    const DXGI_SWAP_CHAIN_DESC1* pDesc,
//    IDXGIOutput* pRestrictToOutput,
//    IDXGISwapChain1** ppSwapChain)
//{
//    LOG("--- factory2 -> create swap chain for core window");
//    auto hr = original(pDevice, pWindow, pDesc, pRestrictToOutput, ppSwapChain);
//
//    if (SUCCEEDED(hr) && ppSwapChain && *ppSwapChain)
//    {
//        IDXGISwapChain* sc = *ppSwapChain;
//        void** vtable = *(void***)(sc);
//        void* present_addr = vtable[8];
//
//        if (!m_master.check(present_addr, DXGI_SWAP_CHAIN_PRESENT))
//            return hr;
//
//        m_hooking.add_hook<DXGI_SWAP_CHAIN_PRESENT>((uintptr_t)present_addr, m_master.dxgi_swap_chain_present);
//    }
//
//    return hr;
//}
//
//HRESULT __fastcall idxgi_factory2_create_swap_chain_for_composition_hook::operator()(
//    idxgi_factory2_create_swap_chain_for_composition_t original,
//    IUnknown* pDevice,
//    const DXGI_SWAP_CHAIN_DESC1* pDesc,
//    IDXGIOutput* pRestrictToOutput,
//    IDXGISwapChain1** ppSwapChain)
//{
//    LOG("--- factory2 -> create swap chain for composition");
//    auto hr = original(pDevice, pDesc, pRestrictToOutput, ppSwapChain);
//
//    if (SUCCEEDED(hr) && ppSwapChain && *ppSwapChain)
//    {
//        IDXGISwapChain* sc = *ppSwapChain;
//        void** vtable = *(void***)(sc);
//        void* present_addr = vtable[8];
//
//        if (!m_master.check(present_addr, DXGI_SWAP_CHAIN_PRESENT))
//            return hr;
//
//        m_hooking.add_hook<DXGI_SWAP_CHAIN_PRESENT>((uintptr_t)present_addr, m_master.dxgi_swap_chain_present);
//    }
//
//    return hr;
//}
//
//HRESULT dxgi_swap_chain_present_hook::operator()(dxgi_swap_chain_present_t original, uintptr_t self, UINT SyncInterval, UINT Flags)
//{
//    LOG(" ---=== SUCCESS ===--- ");
//    auto hr = original(self, SyncInterval, Flags);
//    return hr;
//}
//
//void dxgi_hooking::create_f2_hooks(IUnknown* factory)
//{
//    if (!factory)
//        return;
//
//    IDXGIFactory2* f2 = nullptr;
//    if (FAILED(factory->QueryInterface(__uuidof(IDXGIFactory2), (void**)&f2)))
//    {
//        LOG("Failed to query interface");
//        return;
//    }
//
//    void** vtable = *(void***)(f2);
//
//    // IDXGIFactory2::CreateSwapChain
//    void* target = vtable[10];
//    if (check(target, IDXGI_FACTORY2_CREATE_SWAP_CHAIN))
//        m_hk.add_hook<IDXGI_FACTORY2_CREATE_SWAP_CHAIN>((uintptr_t)target, idxgi_factory2_create_swap_chain);
//
//    // IDXGIFactory2::CreateSwapChainForHwnd
//    target = vtable[15];
//    if (check(target, IDXGI_FACTORY2_CREATE_SWAP_CHAIN_FOR_HWND))
//        m_hk.add_hook<IDXGI_FACTORY2_CREATE_SWAP_CHAIN_FOR_HWND>((uintptr_t)target, idxgi_factory2_create_swap_chain_for_hwnd);
//
//    // IDXGIFactory2::CreateSwapChainForCoreWindow
//    target = vtable[16];
//    if (check(target, IDXGI_FACTORY2_CREATE_SWAP_CHAIN_FOR_CORE_WINDOW))
//        m_hk.add_hook<IDXGI_FACTORY2_CREATE_SWAP_CHAIN_FOR_CORE_WINDOW>((uintptr_t)target, idxgi_factory2_create_swap_chain_for_core_window);
//
//    // IDXGIFactory2::CreateSwapChainForComposition
//    target = vtable[24];
//    if (check(target, IDXGI_FACTORY2_CREATE_SWAP_CHAIN_FOR_COMPOSITION))
//        m_hk.add_hook<IDXGI_FACTORY2_CREATE_SWAP_CHAIN_FOR_COMPOSITION>((uintptr_t)target, idxgi_factory2_create_swap_chain_for_composition);
//
//    f2->Release();
//}
//
//bool dxgi_hooking::check(void* addr, function_id_t id)
//{
//    if (!is_inside_module(addr))
//    {
//        LOG("{} is not inside dxgi.dll module - skipping", (uint32_t)id);
//        return false;
//    }
//
//    if (m_hk.has_hook(id))
//    {
//        return false;
//    }
//
//    return true;
//}
//
//void dxgi_hooking::init(const char* module_path)
//{
//    if (NULL == m_dxgi_handle)
//    {
//        LOG("Failed to get dxgi.dll handle!");
//        return;
//    }
//
//    LOG("dxgi.dll handle is {:x}", (uintptr_t)m_dxgi_handle);
//
//    auto proc_addr = (create_dxgi_factory_t)GetProcAddress(m_dxgi_handle, "CreateDXGIFactory");
//    if (NULL == proc_addr) {
//        LOG("Failed to get {} address!", "CreateDXGIFactory");
//        return;
//    }
//
//    if (!check(proc_addr, CREATE_DXGI_FACTORY))
//        return;
//
//    m_hk.add_hook<CREATE_DXGI_FACTORY>((uintptr_t)proc_addr, create_dxgi_factory);
//
//    LOG("--- a");
//	if (!try_add_hook_by_name<CREATE_DXGI_FACTORY1, create_dxgi_factory1_t >("CreateDXGIFactory1", create_dxgi_factory1))
//        return;
//
//    LOG("--- b");
//	if (!try_add_hook_by_name<CREATE_DXGI_FACTORY2, create_dxgi_factory2_t>("CreateDXGIFactory2", create_dxgi_factory2))
//        return;
//}
//
//bool dxgi_hooking::is_inside_module(void* addr)
//{
//    HMODULE actual_module = nullptr;
//    GetModuleHandleExA(
//        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
//        reinterpret_cast<LPCSTR>(addr),
//        &actual_module
//    );
//
//    if (m_dxgi_handle && actual_module && m_dxgi_handle == actual_module)
//        return true;
//
//    char filename[MAX_PATH]{};
//    GetModuleFileNameA(actual_module, filename, MAX_PATH);
//
//    LOG("Address expected to be in {:x} but found in {}", (uintptr_t)m_dxgi_handle, filename);
//    return false;
//}
//