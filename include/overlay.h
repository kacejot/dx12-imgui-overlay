#pragma once
#include <Windows.h>
#include <dxgi1_4.h>

namespace overlay {

using render_callback_t = void(*)(IDXGISwapChain3*, UINT, UINT);
using font_config_callback_t = void(*)();

void init();
void deinit();
void set_render_callback(render_callback_t render_callback);
void set_font_config_callback(font_config_callback_t callback);
void set_toggle_key(UINT vk_code = VK_OEM_MINUS);
void enable_logging(const char* path = "dx12_overlay_log.txt");
void disable_logging();

}
