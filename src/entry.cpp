#include "hooking.h"
#include "dxgi_hooking.h"

#include <filesystem>
#include <array>

struct master
{
	master(HMODULE dxgi_handle) : hk(0), dxgi(hk, dxgi_handle)
    {
        hk.init();
		dxgi.init();
    }

    hooking hk;
    dxgi_hooking dxgi;
};

std::unique_ptr<master> g_master;

HMODULE find_handle();

extern "C" void init_plugin(HMODULE h_module)
{
    g_master = std::make_unique<master>(find_handle());
}

extern "C" void deinit_plugin()
{
    g_master.reset();
}

HMODULE find_handle()
{
    std::array<char, MAX_PATH> buf;
    if (GetSystemDirectoryA(buf.data(), buf.size()) == 0) {
        return NULL;
    }

    auto system_dxgi_path = std::filesystem::path{ buf.data() } / "dxgi.dll";
    return GetModuleHandleA(system_dxgi_path.generic_string().c_str());
}
