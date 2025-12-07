#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>

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

class Main
{
public:

    Main() : m_hooking(0)
    {
    }

    hooking m_hooking;
};

Main* g_main;

void init()
{
    auto& h = g_main->m_hooking;
    h.init();

    char buf[MAX_PATH];
    if (GetSystemDirectoryA(&buf[0], ARRAYSIZE(buf)) == 0) {
        LOG("Unable to find system dll directory! We're probably about to crash.");
        return;
    }

    auto system_dxgi_path = std::filesystem::path{ buf } / "dxgi.dll";
    auto dxgi_handle = GetModuleHandleA(system_dxgi_path.generic_string().c_str());

    if (FAILED(dxgi_handle))
    {
        LOG("Failed to load dxgi.dll");
        return;
    }

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
