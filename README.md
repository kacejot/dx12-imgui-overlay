# dx12-imgui-overlay

A lightweight C++ library for rendering Dear ImGui overlays on DirectX 12 applications **without creating a new window**. Hooks into an existing DX12 swap chain by intercepting DXGI factory and swap chain creation.

## Features

- **No new window** — renders directly onto the target application's existing swap chain
- **Automatic DX12 device capture** — hooks `CreateDXGIFactory`, `CreateSwapChain`, and `Present`
- **ImGui integration** — provides a simple callback for your ImGui rendering code
- **Minimal API** — just `init()`, `set_render_callback()`, and `deinit()`
- **Toggle visibility** — built-in hotkey support (default: `-` key)
- **CMake integration** — easy to add as a subdirectory

## Use Cases

- Game overlays and HUDs
- Debug/development tools for DX12 applications
- Performance monitors
- Mod menus

## Quick Start

```cpp
#include <overlay.h>
#include <imgui.h>

// Call early, before target app creates its D3D12 device
overlay::init();

// Set your ImGui rendering callback
overlay::set_render_callback([](IDXGISwapChain3*, UINT, UINT) {
    ImGui::Begin("My Overlay");
    ImGui::Text("Hello from the overlay!");
    ImGui::End();
});

// On shutdown
overlay::deinit();
```

## API Reference

```cpp
namespace overlay {

// Initialize hooks (call before DX12 device creation)
void init();

// Cleanup and restore original functions
void deinit();

// Set callback for ImGui rendering (called between NewFrame and Render)
using render_callback_t = void(*)(IDXGISwapChain3*, UINT sync_interval, UINT flags);
void set_render_callback(render_callback_t callback);

// Set the key to toggle overlay visibility (default: VK_OEM_MINUS)
void set_toggle_key(UINT vk_code = VK_OEM_MINUS);

// Optional: enable file logging for debugging
void enable_logging(const char* path = "dx12_overlay_log.txt");
void disable_logging();

}
```

## Integration

### As a CMake subdirectory

```cmake
add_subdirectory(deps/dx12-imgui-overlay)

target_link_libraries(your_target PRIVATE dx12-imgui-overlay)
# minhook and imgui are automatically linked via PUBLIC dependency
```

### Dependencies

This library bundles and links:
- [MinHook](https://github.com/TsudaKageyu/minhook) — API hooking
- [Dear ImGui](https://github.com/ocornut/imgui) — with DX12 backend

If you use this as a subdirectory, your project automatically gets access to both.

## Keywords

DirectX 12 overlay, DX12 ImGui hook, DXGI swap chain hook, IDXGISwapChain Present hook, game overlay library, DX12 rendering injection, ImGui DirectX 12, D3D12 overlay, hook existing DX12 device, no new window overlay

## License

MIT License — see [LICENSE](LICENSE)

This project uses third-party libraries under their respective licenses — see [THIRD_PARTY_LICENSES](THIRD_PARTY_LICENSES)

