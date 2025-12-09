#include "dxgi_hooking.h"

#include <dxgi.h>
#include <dxgi1_2.h>
#include <dxgi1_3.h>
#include <dxgi1_4.h>
#include <dxgi1_5.h>
#include <d3d10_1.h>
#include <d3d11.h>
#include <d3d12.h>

#include <imgui.h>
#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_win32.h>

HRESULT create_dxgi_factory_hook::operator()(create_dxgi_factory_t original, const IID& riid, void** ppFactory)
{
    LOG(" --- create factory");
    HRESULT hr = original(riid, ppFactory);
    if (FAILED(hr) || ppFactory == nullptr || *ppFactory == nullptr)
        return hr;

    void* factory = *ppFactory;
    void** vtable = *(void***)(factory);
    m_master.try_add_hook<IDXGI_FACTORY1_CREATE_SWAP_CHAIN>(
        vtable[10],
		m_master.idxgi_factory1_create_swap_chain);

    m_master.create_f2_hooks((IUnknown*)*ppFactory);
    return hr;
}

HRESULT create_dxgi_factory1_hook::operator()(create_dxgi_factory_t original, const IID& riid, void** ppFactory)
{
    LOG(" --- create factory1");
    HRESULT hr = original(riid, ppFactory);
    if (FAILED(hr) || ppFactory == nullptr || *ppFactory == nullptr)
        return hr;

    void* factory = *ppFactory;
    void** vtable = *(void***)(factory);

    m_master.try_add_hook<IDXGI_FACTORY1_CREATE_SWAP_CHAIN>(
		vtable[10],
		m_master.idxgi_factory1_create_swap_chain);

    m_master.create_f2_hooks((IUnknown*)*ppFactory);
    return hr;
}


HRESULT create_dxgi_factory2_hook::operator()(create_dxgi_factory2_t original, uint32_t flags, const IID& riid, void** ppFactory)
{
    LOG(" --- create factory2");
    HRESULT hr = original(flags, riid, ppFactory);

    if (FAILED(hr) || !ppFactory || !*ppFactory)
        return hr;

    m_master.create_f2_hooks((IUnknown*)(*ppFactory));
    return hr;
}

HRESULT __fastcall idxgi_factory_create_swap_chain_hook::operator()(idxgi_factory_create_swap_chain_t original, uintptr_t self, IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain)
{
    LOG(" --- factory -> create swap chain");
    auto hr = original(self, pDevice, pDesc, ppSwapChain);

    if (SUCCEEDED(hr) && ppSwapChain && *ppSwapChain)
    {
        IDXGISwapChain* sc = *ppSwapChain;
        void** vtable = *(void***)(sc);

        m_master.try_add_hook<DXGI_SWAP_CHAIN_PRESENT>(
            vtable[8],
			m_master.dxgi_swap_chain_present);
    };

    return hr;
}

HRESULT __fastcall idxgi_factory1_create_swap_chain_hook::operator()(idxgi_factory_create_swap_chain_t original, uintptr_t self, IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain)
{
    LOG(" --- factory1 -> create swap chain");
    auto hr = original(self, pDevice, pDesc, ppSwapChain);

    if (SUCCEEDED(hr) && ppSwapChain && *ppSwapChain)
    {
        IDXGISwapChain* sc = *ppSwapChain;
        void** vtable = *(void***)(sc);

        m_master.try_add_hook<DXGI_SWAP_CHAIN_PRESENT>(
            vtable[8],
			m_master.dxgi_swap_chain_present);
    }
    return hr;
}

HRESULT __fastcall idxgi_factory2_create_swap_chain_hook::operator()(
    idxgi_factory_create_swap_chain_t original,
    uintptr_t self,
    IUnknown* pDevice,
    DXGI_SWAP_CHAIN_DESC* pDesc,
    IDXGISwapChain** ppSwapChain)
{
    LOG(" --- factory2 -> create swap chain");
    auto hr = original(self, pDevice, pDesc, ppSwapChain);

    if (SUCCEEDED(hr) && ppSwapChain && *ppSwapChain)
    {
        IDXGISwapChain* sc = *ppSwapChain;
        void** vtable = *(void***)(sc);

        m_master.try_add_hook<DXGI_SWAP_CHAIN_PRESENT>(
            vtable[8],
			m_master.dxgi_swap_chain_present);
    }
    return hr;
}

HRESULT __fastcall idxgi_factory2_create_swap_chain_for_hwnd_hook::operator()(
    idxgi_factory2_create_swap_chain_for_hwnd_t original,
    IUnknown* pDevice,
    HWND hWnd,
    const DXGI_SWAP_CHAIN_DESC1* pDesc,
    const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc,
    IDXGIOutput* pRestrictToOutput,
    IDXGISwapChain1** ppSwapChain)
{
    LOG(" --- factory2 -> create swap chain for hwnd");
    auto hr = original(pDevice, hWnd, pDesc, pFullscreenDesc, pRestrictToOutput, ppSwapChain);

    if (SUCCEEDED(hr) && ppSwapChain && *ppSwapChain)
    {
        IDXGISwapChain* sc = *ppSwapChain;
        void** vtable = *(void***)(sc);
        void* present_addr = vtable[8];

		if (!m_master.try_add_hook<DXGI_SWAP_CHAIN_PRESENT>(present_addr, m_master.dxgi_swap_chain_present))
            return hr;
    }

    return hr;
}

HRESULT __fastcall idxgi_factory2_create_swap_chain_for_core_window_hook::operator()(
    idxgi_factory2_create_swap_chain_for_core_window_t original,
    IUnknown* pDevice,
    IUnknown* pWindow,
    const DXGI_SWAP_CHAIN_DESC1* pDesc,
    IDXGIOutput* pRestrictToOutput,
    IDXGISwapChain1** ppSwapChain)
{
    LOG(" --- factory2 -> create swap chain for core window");
    auto hr = original(pDevice, pWindow, pDesc, pRestrictToOutput, ppSwapChain);

    if (SUCCEEDED(hr) && ppSwapChain && *ppSwapChain)
    {
        IDXGISwapChain* sc = *ppSwapChain;
        void** vtable = *(void***)(sc);

        m_master.try_add_hook<DXGI_SWAP_CHAIN_PRESENT>(
            vtable[8],
			m_master.dxgi_swap_chain_present);
    }

    return hr;
}

HRESULT __fastcall idxgi_factory2_create_swap_chain_for_composition_hook::operator()(
    idxgi_factory2_create_swap_chain_for_composition_t original,
    IUnknown* pDevice,
    const DXGI_SWAP_CHAIN_DESC1* pDesc,
    IDXGIOutput* pRestrictToOutput,
    IDXGISwapChain1** ppSwapChain)
{
    LOG(" --- factory2 -> create swap chain for composition");
    auto hr = original(pDevice, pDesc, pRestrictToOutput, ppSwapChain);

    if (SUCCEEDED(hr) && ppSwapChain && *ppSwapChain)
    {
        IDXGISwapChain* sc = *ppSwapChain;
        void** vtable = *(void***)(sc);

        m_master.try_add_hook<DXGI_SWAP_CHAIN_PRESENT>(
            vtable[8],
			m_master.dxgi_swap_chain_present);
    }

    return hr;
}


HWND hwnd = NULL;
ID3D11Device* device;
ID3D11DeviceContext* context;
ID3D11RenderTargetView* targetView;
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

HRESULT dxgi_swap_chain_present_hook::operator()(dxgi_swap_chain_present_t original, IDXGISwapChain* self, UINT SyncInterval, UINT Flags)
{
    auto hr = original(self, SyncInterval, Flags);
    if (FAILED(hr))
        return hr;

    if (hwnd == NULL)
    {
		LOG("  --- trying to find window");
        hwnd = FindWindowA("UnrealWindow", "S.T.A.L.K.E.R. 2: Heart of Chornobyl  ");
        if (NULL == hwnd)
        {
            LOG(" xxx Failed to find window!");
            hwnd = GetForegroundWindow();
            return hr;
        }

        void* wnd_proc = reinterpret_cast<void*>(GetWindowLongPtrW(hwnd, GWLP_WNDPROC));
        if (NULL == wnd_proc)
            LOG(" xxx Failed to get window procedure!");

        m_master.m_hk->add_hook<WND_PROC>(
            (uintptr_t)wnd_proc,
            [](WNDPROC original, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) -> LRESULT
            {
                LOG(" --- window proc");

				auto result = original(hWnd, msg, wParam, lParam);

                if (result)
					return result;

                if (hWnd != hwnd)
                    return result;

                ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);;
			});
        LOG(" --- found window & hooked window");
    }

    if (!device) {
        LOG("  --- trying to init dx11 imgui");
        ID3D11Texture2D* renderTarget = nullptr;

        LOG("  --- 1");
        auto try_get = [&](const char* name, const IID& iid) -> bool {
            void* dev = nullptr;
            HRESULT hr = self->GetDevice(iid, &dev);
            LOG("GetDevice({}) -> hr=0x{:X}, dev={}", name, (uint32_t)hr, dev);
            if (SUCCEEDED(hr) && dev != nullptr)
            {
                LOG("SUCCESS: swapchain is backed by {}", name);
                ((IUnknown*)dev)->Release();
                return true;
            }
            return false;
            };

        // D3D12
        uint8_t result = 0;
        result |= try_get("ID3D12Device", __uuidof(ID3D12Device));

        // D3D11
        result |= try_get("ID3D11Device", __uuidof(ID3D11Device));

        // D3D10.x
        result |= try_get("ID3D10Device", __uuidof(ID3D10Device));
        result |= try_get("ID3D10Device1", __uuidof(ID3D10Device1));

        // DXGI-level interfaces
        result |= try_get("IDXGIDevice", __uuidof(IDXGIDevice));
        result |= try_get("IDXGIDevice1", __uuidof(IDXGIDevice1));
        result |= try_get("IDXGIDevice2", __uuidof(IDXGIDevice2));
        result |= try_get("IDXGIDevice3", __uuidof(IDXGIDevice3));
        result |= try_get("IDXGIDevice4", __uuidof(IDXGIDevice4));

        if (!result)
            return hr;

        LOG("  --- 2");
        device->GetImmediateContext(&context);

        LOG("  --- 3");
        self->GetBuffer(0, __uuidof(renderTarget), reinterpret_cast<void**>(&renderTarget));

        LOG("  --- 4");
        device->CreateRenderTargetView(renderTarget, nullptr, &targetView);

        LOG("  --- 5");
        renderTarget->Release();

        LOG("  --- 6");
        ImGui::CreateContext();

        LOG("  --- 7");
        ImGuiIO& io = ImGui::GetIO();

        LOG("  --- 8");
        ImGui::StyleColorsLight();

        LOG("  --- 9");
        ImGui_ImplWin32_Init(hwnd);

        LOG("  --- 10");
        ImGui_ImplDX11_Init(device, context);
    }
    else
    {
        LOG(" --- drawing ");
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowSize({ 800.f, 500.f });
        ImGui::Begin("Debug");

        ImGui::Text("Awesome!");

        ImGui::End();

        ImGui::Render();
        context->OMSetRenderTargets(1, &targetView, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    return hr;
}

void dxgi_hooking::create_f2_hooks(IUnknown* factory)
{
    if (!factory)
        return;

    IDXGIFactory2* f2 = nullptr;
    if (FAILED(factory->QueryInterface(__uuidof(IDXGIFactory2), (void**)&f2)))
    {
        LOG("Failed to query interface");
        return;
    }

    void** vtable = *(void***)(f2);

    void* target = vtable[10];
    if (check(target, IDXGI_FACTORY2_CREATE_SWAP_CHAIN))
        m_hk->add_hook<IDXGI_FACTORY2_CREATE_SWAP_CHAIN>((uintptr_t)target, idxgi_factory2_create_swap_chain);

    target = vtable[15];
    if (check(target, IDXGI_FACTORY2_CREATE_SWAP_CHAIN_FOR_HWND))
        m_hk->add_hook<IDXGI_FACTORY2_CREATE_SWAP_CHAIN_FOR_HWND>((uintptr_t)target, idxgi_factory2_create_swap_chain_for_hwnd);

    target = vtable[16];
    if (check(target, IDXGI_FACTORY2_CREATE_SWAP_CHAIN_FOR_CORE_WINDOW))
        m_hk->add_hook<IDXGI_FACTORY2_CREATE_SWAP_CHAIN_FOR_CORE_WINDOW>((uintptr_t)target, idxgi_factory2_create_swap_chain_for_core_window);

    target = vtable[24];
    if (check(target, IDXGI_FACTORY2_CREATE_SWAP_CHAIN_FOR_COMPOSITION))
        m_hk->add_hook<IDXGI_FACTORY2_CREATE_SWAP_CHAIN_FOR_COMPOSITION>((uintptr_t)target, idxgi_factory2_create_swap_chain_for_composition);

    f2->Release();
}

bool dxgi_hooking::check(void* addr, function_id_t id)
{
    if (!is_inside_module(addr))
    {
        LOG("{} is not inside dxgi.dll module - skipping", (uint32_t)id);
        return false;
    }

    if (m_hk->has_hook(id))
    {
        return false;
    }

    return true;
}

void dxgi_hooking::init()
{
    if (NULL == m_dxgi_handle)
    {
        LOG("Failed to get dxgi.dll handle!");
        return;
    }

    LOG("dxgi.dll handle is {:x}", (uintptr_t)m_dxgi_handle);

	if (!try_add_hook_by_name<CREATE_DXGI_FACTORY, create_dxgi_factory_t>("CreateDXGIFactory", create_dxgi_factory))
        return;

	if (!try_add_hook_by_name<CREATE_DXGI_FACTORY1, create_dxgi_factory1_t >("CreateDXGIFactory1", create_dxgi_factory1))
        return;

	if (!try_add_hook_by_name<CREATE_DXGI_FACTORY2, create_dxgi_factory2_t>("CreateDXGIFactory2", create_dxgi_factory2))
        return;
}

bool dxgi_hooking::is_inside_module(void* addr)
{
    HMODULE actual_module = nullptr;
    GetModuleHandleExA(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCSTR>(addr),
        &actual_module
    );

    if (m_dxgi_handle && actual_module && m_dxgi_handle == actual_module)
        return true;

    char filename[MAX_PATH]{};
    GetModuleFileNameA(actual_module, filename, MAX_PATH);

    LOG("Address expected to be in {:x} but found in {}", (uintptr_t)m_dxgi_handle, filename);
    return false;
}
