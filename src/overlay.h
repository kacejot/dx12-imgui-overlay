//#pragma once
//#include <d3d11.h>
//
//#include <imgui.h>
//#include <backends/imgui_impl_win32.h>
//#include <backends/imgui_impl_dx11.h>
//
//#include "dxgi_hooking.h"
//
//class overlay
//{
//public:
//	void init(dxgi_hooking* dxgi)
//	{
//		m_dxgi = dxgi;
//	}
//
//	template <typename T>
//	void set_widget_callback(T&& callback)
//	{
//		m_widget_callback = std::forward<T>(callback);
//	}
//
//	void render()
//	{
//		ImGui_ImplDX11_NewFrame();
//		ImGui_ImplWin32_NewFrame();
//		ImGui::NewFrame();
//
//		m_widget_callback();
//
//		ImGui::Render();
//		context->OMSetRenderTargets(1, &targetView, nullptr);
//		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
//	}
//
//private:
//	ID3D11Device* device = nullptr;
//	ID3D11DeviceContext* context = nullptr;
//	ID3D11RenderTargetView* targetView = nullptr;
//
//	dxgi_hooking* m_dxgi = nullptr;
//	HWND hwnd = NULL;
//	decltype([] {}) m_widget_callback = {};
//
//};
//