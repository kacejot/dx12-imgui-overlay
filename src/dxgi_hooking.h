#pragma once
#include <Windows.h>
#include <dxgi1_2.h>
#include <cstdint>

#include "hooking.h"
#include "log.h"

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
    DXGI_SWAP_CHAIN_PRESENT = 9,
	WND_PROC = 10,
};

class dxgi_hooking;

using create_dxgi_factory_t = HRESULT(*)(const IID& riid, void** ppFactory);
class create_dxgi_factory_hook
{
public:
	create_dxgi_factory_hook(dxgi_hooking& master) : m_master(master)
    {
    }

    HRESULT operator()(create_dxgi_factory_t original, const IID& riid, void** ppFactory);

private:
	dxgi_hooking& m_master;
};

using create_dxgi_factory1_t = HRESULT(*)(const IID& riid, void** ppFactory);
class create_dxgi_factory1_hook
{
public:
    create_dxgi_factory1_hook(dxgi_hooking& master) : m_master(master)
    {
    }

    HRESULT operator()(create_dxgi_factory1_t original, const IID& riid, void** ppFactory);

private:
    dxgi_hooking& m_master;
};

using create_dxgi_factory2_t = HRESULT(*)(uint32_t flags, const IID& riid, void** ppFactory);
class create_dxgi_factory2_hook
{
public:
    create_dxgi_factory2_hook(dxgi_hooking& master) : m_master(master)
    {
    }

    HRESULT operator()(create_dxgi_factory2_t original, uint32_t flags, const IID& riid, void** ppFactory);

private:
    dxgi_hooking& m_master;
};

using idxgi_factory_create_swap_chain_t = HRESULT(*)(uintptr_t self, IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain);
class idxgi_factory_create_swap_chain_hook
{
public:
    idxgi_factory_create_swap_chain_hook(dxgi_hooking& master) : m_master(master)
    {
    }

    HRESULT operator()(idxgi_factory_create_swap_chain_t original, uintptr_t self, IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain);

private:
    dxgi_hooking& m_master;
};

class idxgi_factory1_create_swap_chain_hook
{
public:
    idxgi_factory1_create_swap_chain_hook(dxgi_hooking& master) : m_master(master)
    {
    }

    HRESULT operator()(idxgi_factory_create_swap_chain_t original, uintptr_t self, IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain);

private:
    dxgi_hooking& m_master;
};

class idxgi_factory2_create_swap_chain_hook
{
public:
    idxgi_factory2_create_swap_chain_hook(dxgi_hooking& master) : m_master(master)
    {
    }
    HRESULT operator()(idxgi_factory_create_swap_chain_t original, uintptr_t self, IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain);

private:
    dxgi_hooking& m_master;
};

using idxgi_factory2_create_swap_chain_for_hwnd_t = HRESULT(*)(
    IUnknown* pDevice,
    HWND hWnd,
    const DXGI_SWAP_CHAIN_DESC1* pDesc,
    const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc,
    IDXGIOutput* pRestrictToOutput,
    IDXGISwapChain1** ppSwapChain
    );
class idxgi_factory2_create_swap_chain_for_hwnd_hook
{
public:
    idxgi_factory2_create_swap_chain_for_hwnd_hook(dxgi_hooking& master) : m_master(master)
    {
    }

    HRESULT operator()(
        idxgi_factory2_create_swap_chain_for_hwnd_t original,
        IUnknown* pDevice,
        HWND hWnd,
        const DXGI_SWAP_CHAIN_DESC1* pDesc,
        const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc,
        IDXGIOutput* pRestrictToOutput,
		IDXGISwapChain1** ppSwapChain);

private:
    dxgi_hooking& m_master;
};

using idxgi_factory2_create_swap_chain_for_core_window_t = HRESULT(*)(
    IUnknown* pDevice,
    IUnknown* pWindow,
    const DXGI_SWAP_CHAIN_DESC1* pDesc,
    IDXGIOutput* pRestrictToOutput,
    IDXGISwapChain1** ppSwapChain
    );
class idxgi_factory2_create_swap_chain_for_core_window_hook
{
public:
    idxgi_factory2_create_swap_chain_for_core_window_hook(dxgi_hooking& master) : m_master(master)
    {
    }

    HRESULT operator()(
        idxgi_factory2_create_swap_chain_for_core_window_t original,
        IUnknown* pDevice,
        IUnknown* pWindow,
        const DXGI_SWAP_CHAIN_DESC1* pDesc,
        IDXGIOutput* pRestrictToOutput,
        IDXGISwapChain1** ppSwapChain);

private:
    dxgi_hooking& m_master;
};

using idxgi_factory2_create_swap_chain_for_composition_t = HRESULT(*)(
    IUnknown* pDevice,
    const DXGI_SWAP_CHAIN_DESC1* pDesc,
    IDXGIOutput* pRestrictToOutput,
    IDXGISwapChain1** ppSwapChain
    );
class idxgi_factory2_create_swap_chain_for_composition_hook
{
public:
    idxgi_factory2_create_swap_chain_for_composition_hook(dxgi_hooking& master) : m_master(master)
    {
    }

    HRESULT operator()(
        idxgi_factory2_create_swap_chain_for_composition_t original,
        IUnknown* pDevice,
        const DXGI_SWAP_CHAIN_DESC1* pDesc,
        IDXGIOutput* pRestrictToOutput,
		IDXGISwapChain1** ppSwapChain);

private:
    dxgi_hooking& m_master;
};

using on_present_t = std::function<void(IDXGISwapChain*, UINT, UINT)>;
using dxgi_swap_chain_present_t = HRESULT(*)(IDXGISwapChain* self, UINT SyncInterval, UINT Flags);

class dxgi_swap_chain_present_hook
{
public:
    dxgi_swap_chain_present_hook(dxgi_hooking& master, on_present_t payload) : m_master(master), m_on_present(payload)
    {
	}

    HRESULT operator()(dxgi_swap_chain_present_t original, IDXGISwapChain* self, UINT SyncInterval, UINT Flags);

private:
	dxgi_hooking& m_master;
    on_present_t m_on_present;
};

class dxgi_hooking
{
public:
    dxgi_hooking(std::unique_ptr<hooking> hk, HMODULE h, on_present_t&& on_present = {})
        : m_hk(std::move(hk))
        , m_dxgi_handle(h)
        , create_dxgi_factory(*this)
        , create_dxgi_factory1(*this)
        , create_dxgi_factory2(*this)
        , idxgi_factory_create_swap_chain(*this)
        , idxgi_factory1_create_swap_chain(*this)
        , idxgi_factory2_create_swap_chain(*this)
        , idxgi_factory2_create_swap_chain_for_hwnd(*this)
        , idxgi_factory2_create_swap_chain_for_core_window(*this)
        , idxgi_factory2_create_swap_chain_for_composition(*this)
        , dxgi_swap_chain_present(*this, std::move(on_present))
    {
		init();
	}

    template <function_id_t id, typename fn, typename hook_t>
    bool try_add_hook_by_name(const char* fn_name, hook_t&& hook);

    template <function_id_t id, typename hook_t>
    bool try_add_hook(void* proc_addr, hook_t&& hook);

	create_dxgi_factory_hook create_dxgi_factory;
	create_dxgi_factory1_hook create_dxgi_factory1;
	create_dxgi_factory2_hook create_dxgi_factory2;
	idxgi_factory_create_swap_chain_hook idxgi_factory_create_swap_chain;
	idxgi_factory1_create_swap_chain_hook idxgi_factory1_create_swap_chain;
	idxgi_factory2_create_swap_chain_hook idxgi_factory2_create_swap_chain;
	idxgi_factory2_create_swap_chain_for_hwnd_hook idxgi_factory2_create_swap_chain_for_hwnd;
	idxgi_factory2_create_swap_chain_for_core_window_hook idxgi_factory2_create_swap_chain_for_core_window;
	idxgi_factory2_create_swap_chain_for_composition_hook idxgi_factory2_create_swap_chain_for_composition;
	dxgi_swap_chain_present_hook dxgi_swap_chain_present;
    
    bool check(void* addr, function_id_t id);
    void create_f2_hooks(IUnknown* factory);

private:
	void init();
    bool is_inside_module(void* addr);

public:
    std::unique_ptr<hooking> m_hk;
	HMODULE m_dxgi_handle;
};

template <function_id_t id, typename fn, typename hook_t>
bool dxgi_hooking::try_add_hook_by_name(const char* fn_name, hook_t&& hook)
{
    auto proc_addr = (fn)GetProcAddress(m_dxgi_handle, fn_name);
    if (NULL == proc_addr) {
        LOG("Failed to get {} address!", fn_name);
        return false;
    }

    return try_add_hook<id>(proc_addr, std::forward<hook_t>(hook));
}

template <function_id_t id, typename hook_t>
bool dxgi_hooking::try_add_hook(void* proc_addr, hook_t&& hook)
{
    if (!check(proc_addr, id))
        return false;
    
    m_hk->add_hook<id>((uintptr_t)proc_addr, std::forward<hook_t>(hook));
    return true;
}
