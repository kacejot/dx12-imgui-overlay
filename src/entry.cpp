#include "hooking.h"
#include "dxgi_hooking.h"

#include <filesystem>

std::unique_ptr<dxgi_hooking> dxgi;

extern "C" void init_plugin(HMODULE h_module)
{    
    char buf[MAX_PATH];
    if (GetSystemDirectoryA(&buf[0], ARRAYSIZE(buf)) == 0) {
        return;
    }
    
    auto system_dxgi_path = std::filesystem::path{ buf } / "dxgi.dll";
    auto handle = GetModuleHandleA(system_dxgi_path.generic_string().c_str());

	auto hk = std::make_unique<hooking>(0);
    hk->init();

    dxgi = std::make_unique<dxgi_hooking>(std::move(hk), handle);
}

extern "C" void deinit_plugin()
{
    dxgi.reset();
}
