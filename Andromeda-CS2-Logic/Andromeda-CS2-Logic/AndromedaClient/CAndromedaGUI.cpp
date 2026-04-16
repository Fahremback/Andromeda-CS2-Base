#include "CAndromedaGUI.hpp"

#include <ShlObj_core.h>

#include <ImGui/imgui_impl_win32.h>
#include <ImGui/imgui_impl_dx11.h>

#include <DllLauncher.hpp>
#include <Common/Helpers/StringHelper.hpp>

#include <CS2/SDK/SDK.hpp>
#include <AndromedaClient/GUI/CAndromedaMenu.hpp>
#include <AndromedaClient/Fonts/CFontManager.hpp>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler( HWND hWnd , UINT msg , WPARAM wParam , LPARAM lParam );

static CAndromedaGUI g_AndromedaGUI{};

IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler( HWND hwnd , UINT msg , WPARAM wParam , LPARAM lParam );

auto CAndromedaGUI::GetFontManager() -> CFontManager*
{
	return ::GetFontManager();
}

auto CAndromedaGUI::OnInit( IDXGISwapChain* pSwapChain ) -> void
{
	if ( m_bInit )
		return;

	if ( FAILED( pSwapChain->GetDevice( __uuidof( ID3D11Device ) , (void**)&m_pDevice ) ) )
		return;

	m_pDevice->GetImmediateContext( &m_pDeviceContext );

	DXGI_SWAP_CHAIN_DESC sd;
	pSwapChain->GetDesc( &sd );

	m_hCS2Window = sd.OutputWindow;

	m_pImGuiContext = ImGui::CreateContext();

	ImGui_ImplWin32_Init( m_hCS2Window );
	ImGui_ImplDX11_Init( m_pDevice , m_pDeviceContext );

	m_GuiFile = GetDllDir() + XorStr( "Andromeda.json" );

	InitFont();
	UpdateStyle();

	m_bInit = true;
}

auto CAndromedaGUI::OnDestroy() -> void
{
	if ( !m_bInit )
		return;

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext( m_pImGuiContext );

	m_bInit = false;
}

auto CAndromedaGUI::InitFont() -> void
{
	GetFontManager()->OnInit();
}

auto CAndromedaGUI::UpdateStyle() -> void
{
	SetModernRedesignStyle();
}

auto CAndromedaGUI::SetIndigoStyle() -> void
{
	ImGuiStyle& style = ImGui::GetStyle();

	style.WindowPadding = ImVec2( 15 , 15 );
	style.WindowRounding = 5.0f;
	style.FramePadding = ImVec2( 5 , 5 );
	style.FrameRounding = 4.0f;
	style.ItemSpacing = ImVec2( 12 , 8 );
	style.ItemInnerSpacing = ImVec2( 8 , 6 );
	style.IndentSpacing = 25.0f;
	style.ScrollbarSize = 15.0f;
	style.ScrollbarRounding = 9.0f;
	style.GrabMinSize = 5.0f;
	style.GrabRounding = 3.0f;

	style.Colors[ImGuiCol_Text] = ImVec4( 0.80f , 0.80f , 0.83f , 1.00f );
	style.Colors[ImGuiCol_TextDisabled] = ImVec4( 0.24f , 0.23f , 0.29f , 1.00f );
	style.Colors[ImGuiCol_WindowBg] = ImVec4( 0.06f , 0.05f , 0.07f , 1.00f );
	style.Colors[ImGuiCol_ChildBg] = ImVec4( 0.07f , 0.07f , 0.09f , 1.00f );
	style.Colors[ImGuiCol_PopupBg] = ImVec4( 0.07f , 0.07f , 0.09f , 1.00f );
	style.Colors[ImGuiCol_Border] = ImVec4( 0.80f , 0.80f , 0.83f , 0.88f );
	style.Colors[ImGuiCol_BorderShadow] = ImVec4( 0.92f , 0.91f , 0.88f , 0.00f );
	style.Colors[ImGuiCol_FrameBg] = ImVec4( 0.10f , 0.09f , 0.12f , 1.00f );
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4( 0.24f , 0.23f , 0.29f , 1.00f );
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4( 0.56f , 0.56f , 0.58f , 1.00f );
	style.Colors[ImGuiCol_TitleBg] = ImVec4( 0.10f , 0.09f , 0.12f , 1.00f );
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4( 1.00f , 0.98f , 0.95f , 0.75f );
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4( 0.07f , 0.07f , 0.09f , 1.00f );
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4( 0.10f , 0.09f , 0.12f , 1.00f );
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4( 0.10f , 0.09f , 0.12f , 1.00f );
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4( 0.80f , 0.80f , 0.83f , 0.31f );
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4( 0.56f , 0.56f , 0.58f , 1.00f );
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4( 0.06f , 0.05f , 0.07f , 1.00f );
	style.Colors[ImGuiCol_CheckMark] = ImVec4( 0.80f , 0.80f , 0.83f , 0.31f );
	style.Colors[ImGuiCol_SliderGrab] = ImVec4( 0.80f , 0.80f , 0.83f , 0.31f );
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4( 0.06f , 0.05f , 0.07f , 1.00f );
	style.Colors[ImGuiCol_Button] = ImVec4( 0.10f , 0.09f , 0.12f , 1.00f );
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4( 0.24f , 0.23f , 0.29f , 1.00f );
	style.Colors[ImGuiCol_ButtonActive] = ImVec4( 0.56f , 0.56f , 0.58f , 1.00f );
	style.Colors[ImGuiCol_Header] = ImVec4( 0.10f , 0.09f , 0.12f , 1.00f );
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4( 0.56f , 0.56f , 0.58f , 1.00f );
	style.Colors[ImGuiCol_HeaderActive] = ImVec4( 0.06f , 0.05f , 0.07f , 1.00f );
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4( 0.00f , 0.00f , 0.00f , 0.00f );
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4( 0.56f , 0.56f , 0.58f , 1.00f );
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4( 0.06f , 0.05f , 0.07f , 1.00f );
	style.Colors[ImGuiCol_PlotLines] = ImVec4( 0.40f , 0.39f , 0.38f , 0.63f );
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4( 0.25f , 1.00f , 0.00f , 1.00f );
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4( 0.40f , 0.39f , 0.38f , 0.63f );
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4( 0.25f , 1.00f , 0.00f , 1.00f );
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4( 0.25f , 1.00f , 0.00f , 0.43f );
}

auto CAndromedaGUI::SetVermillionStyle() -> void
{

}

auto CAndromedaGUI::SetClassicSteamStyle() -> void
{

}

auto CAndromedaGUI::SetModernRedesignStyle() -> void
{
	ImGuiStyle& style = ImGui::GetStyle();

	style.WindowPadding = ImVec2( 0 , 0 );
	style.WindowRounding = 8.f;
	style.ChildRounding = 6.f;
	style.FrameRounding = 4.f;
	style.GrabRounding = 4.f;
	style.PopupRounding = 4.f;
	style.ScrollbarRounding = 4.f;

	style.Colors[ImGuiCol_WindowBg] = ImVec4( 0.07f , 0.09f , 0.13f , 1.0f );
	style.Colors[ImGuiCol_ChildBg] = ImVec4( 0.09f , 0.11f , 0.16f , 1.0f );
	style.Colors[ImGuiCol_PopupBg] = ImVec4( 0.09f , 0.11f , 0.16f , 1.0f );
	style.Colors[ImGuiCol_Border] = ImVec4( 0.16f , 0.20f , 0.28f , 1.0f );
	style.Colors[ImGuiCol_FrameBg] = ImVec4( 0.13f , 0.16f , 0.23f , 1.0f );
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4( 0.16f , 0.20f , 0.28f , 1.0f );
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4( 0.13f , 0.16f , 0.23f , 1.0f );
	style.Colors[ImGuiCol_TitleBg] = ImVec4( 0.07f , 0.09f , 0.13f , 1.0f );
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4( 0.07f , 0.09f , 0.13f , 1.0f );
	style.Colors[ImGuiCol_Button] = ImVec4( 0.13f , 0.16f , 0.23f , 1.0f );
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4( 0.16f , 0.20f , 0.28f , 1.0f );
	style.Colors[ImGuiCol_ButtonActive] = ImVec4( 0.13f , 0.16f , 0.23f , 1.0f );
	style.Colors[ImGuiCol_CheckMark] = ImVec4( 0.13f , 0.43f , 0.82f , 1.0f );
	style.Colors[ImGuiCol_SliderGrab] = ImVec4( 0.13f , 0.43f , 0.82f , 1.0f );
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4( 0.16f , 0.50f , 0.90f , 1.0f );
}

auto CAndromedaGUI::OnPresent( IDXGISwapChain* pSwapChain ) -> void
{
	if ( !m_bInit )
	{
		OnInit( pSwapChain );
		return;
	}

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	OnRender( pSwapChain );

	ImGui::Render();

	m_pDeviceContext->OMSetRenderTargets( 1 , &m_pRenderTargetView , nullptr );
	ImGui_ImplDX11_RenderDrawData( ImGui::GetDrawData() );
}

auto CAndromedaGUI::OnResizeBuffers( IDXGISwapChain* pSwapChain ) -> void
{
	ClearRenderTargetView();
}

auto CAndromedaGUI::OnRender( IDXGISwapChain* pSwapChain ) -> void
{
	if ( !m_pRenderTargetView )
	{
		ID3D11Texture2D* pBackBuffer = nullptr;
		if ( FAILED( pSwapChain->GetBuffer( 0 , __uuidof( ID3D11Texture2D ) , (void**)&pBackBuffer ) ) )
			return;

		m_pDevice->CreateRenderTargetView( pBackBuffer , nullptr , &m_pRenderTargetView );
		pBackBuffer->Release();
	}

	if ( GetAndromedaGUI()->IsVisible() )
		GetAndromedaMenu()->OnRenderMenu();
}

auto CAndromedaGUI::OnReopenGUI() -> void
{

}

LRESULT WINAPI CAndromedaGUI::GUI_WndProc( HWND hwnd , UINT uMsg , WPARAM wParam , LPARAM lParam )
{
	if ( GetAndromedaGUI()->IsVisible() )
	{
		if ( ImGui_ImplWin32_WndProcHandler( hwnd , uMsg , wParam , lParam ) )
			return true;
	}

	return CallWindowProc( GetAndromedaGUI()->m_WndProc_o , hwnd , uMsg , wParam , lParam );
}

auto CAndromedaGUI::ClearRenderTargetView() -> void
{
	if ( m_pRenderTargetView )
	{
		m_pRenderTargetView->Release();
		m_pRenderTargetView = nullptr;
	}
}

auto GetAndromedaGUI() -> CAndromedaGUI*
{
	return &g_AndromedaGUI;
}
