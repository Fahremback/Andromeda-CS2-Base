#include "CAndromedaGUI.hpp"

#include <ShlObj_core.h>

#include <ImGui/imgui_impl_win32.h>
#include <ImGui/imgui_impl_dx11.h>

#include <DllLauncher.hpp>
#include <Common/Helpers/StringHelper.hpp>

#include <CS2/SDK/SDK.hpp>
#include <CS2/SDK/Interface/IEngineToClient.hpp>
#include <CS2/SDK/SDL3/SDL3_Functions.hpp>

#include <CS2/Hook/Hook_IsRelativeMouseMode.hpp>

#include <AndromedaClient/CAndromedaClient.hpp>
#include <AndromedaClient/Settings/Settings.hpp>

static CAndromedaGUI g_AndromedaGUI{};

IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler( HWND hwnd , UINT msg , WPARAM wParam , LPARAM lParam );

auto CAndromedaGUI::OnInit( IDXGISwapChain* pSwapChain ) -> void
{
	DXGI_SWAP_CHAIN_DESC SwapChainDesc;

	if ( FAILED( pSwapChain->GetDevice( IID_PPV_ARGS( &m_pDevice ) ) ) )
	{
		DEV_LOG( "[error] CAndromedaGUI::OnInit: #1\n" );
		return;
	}

	m_pDevice->GetImmediateContext( &m_pDeviceContext );

	if ( FAILED( pSwapChain->GetDesc( &SwapChainDesc ) ) )
	{
		DEV_LOG( "[error] CAndromedaGUI::OnInit: #2\n" );
		return;
	}

	m_hCS2Window = SwapChainDesc.OutputWindow;

	m_pImGuiContext = ImGui::CreateContext();

	m_GuiFile = GetDllDir() + XorStr( GUI_FILE );

	if ( !m_pFreeType_Font )
		m_pFreeType_Font = new FreeTypeBuild();

	ImGui::SetCurrentContext( m_pImGuiContext );

	ImGui::GetIO().IniFilename = m_GuiFile.c_str();
	ImGui::GetIO().LogFilename = "";

	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
	ImGui::GetIO().BackendFlags |= ImGuiBackendFlags_HasSetMousePos;

	ImGui_ImplWin32_Init( m_hCS2Window );
	ImGui_ImplDX11_Init( m_pDevice , m_pDeviceContext );

	InitFont();
	UpdateStyle();

	m_WndProc_o = (WNDPROC)SetWindowLongPtrA( m_hCS2Window , GWLP_WNDPROC , (LONG_PTR)GUI_WndProc );

	m_bInit = true;
}

auto CAndromedaGUI::OnDestroy() -> void
{
	SetWindowLongPtrA( m_hCS2Window , GWLP_WNDPROC , (LONG_PTR)GetAndromedaGUI()->m_WndProc_o );

	m_bVisible = false;

	if ( m_pFreeType_Font )
	{
		delete m_pFreeType_Font;
		m_pFreeType_Font = nullptr;
	}

	ClearRenderTargetView();

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();

	ImGui::DestroyContext();

	m_bInit = false;
}

auto CAndromedaGUI::InitFont() -> void
{
	ImGuiIO& io = ImGui::GetIO();

	static const ImWchar TahomaRanges[] =
	{
		0x0020, 0xFFFC,
		0,
	};

	wchar_t* szWindowsFontPath = nullptr;

	if ( SHGetKnownFolderPath( FOLDERID_Fonts , 0 , 0 , &szWindowsFontPath ) == S_OK )
	{
		std::wstring TahomaFont = std::wstring( szWindowsFontPath ) + L"\\tahoma.ttf";
		io.Fonts->AddFontFromFileTTF( unicode_to_utf8( TahomaFont ).c_str() , 15.f , nullptr , TahomaRanges );
	}

	CoTaskMemFree( szWindowsFontPath );
}

void CAndromedaGUI::OnPresent( IDXGISwapChain* pSwapChain )
{
	if ( !m_bInit )
		OnInit( pSwapChain );
	else
		OnRender( pSwapChain );
}

void CAndromedaGUI::OnResizeBuffers( IDXGISwapChain* pSwapChain )
{
	OnDestroy();
}
 
void CAndromedaGUI::OnRender( IDXGISwapChain* pSwapChain )
{
	if ( m_pFreeType_Font && m_pFreeType_Font->PreNewFrame() )
	{
		ImGui_ImplDX11_InvalidateDeviceObjects();
		ImGui_ImplDX11_CreateDeviceObjects();
	}
	else
	{
		if ( !m_pRenderTargetView )
		{
			ID3D11Texture2D* pBackBuffer = nullptr;

			pSwapChain->GetBuffer( 0 , IID_PPV_ARGS( &pBackBuffer ) );

			D3D11_RENDER_TARGET_VIEW_DESC RenderTargetDesc;

			memset( &RenderTargetDesc , 0 , sizeof( RenderTargetDesc ) );

			RenderTargetDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			RenderTargetDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;

			if ( pBackBuffer )
			{
				m_pDevice->CreateRenderTargetView( pBackBuffer , &RenderTargetDesc , &m_pRenderTargetView );
				pBackBuffer->Release();
			}
		}

		ImGui::SetCurrentContext( m_pImGuiContext );

		m_pDeviceContext->OMGetRenderTargets( 1 , &m_pMainRenderTarget , 0 );
		m_pDeviceContext->OMSetRenderTargets( 1 , &m_pRenderTargetView , 0 );

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();

		ImGui::NewFrame();

		GetAndromedaClient()->OnRender();

		ImGui::EndFrame();
		ImGui::Render();

		ImGui_ImplDX11_RenderDrawData( ImGui::GetDrawData() );

		m_pDeviceContext->OMSetRenderTargets( 1 , &m_pMainRenderTarget , 0 );
	}
}

auto CAndromedaGUI::OnReopenGUI() -> void
{
	m_bVisible = !m_bVisible;

	ImGui::GetIO().MouseDrawCursor = m_bVisible;
	ShowCursor( !m_bVisible );

	IsRelativeMouseMode_o( SDK::Interfaces::InputSystem() , m_bVisible ? false : m_bMainActive );

	if ( m_bVisible )
	{
		if ( m_vecMousePosSave.x == 0.f && m_vecMousePosSave.y == 0.f )
			m_vecMousePosSave = ImGui::GetIO().DisplaySize / 2.f;

		ImGui::GetIO().MousePos = m_vecMousePosSave;

		if ( SDK::Interfaces::EngineToClient()->IsInGame() )
			GetSDL3Functions()->SDL_WarpMouseInWindow_o( nullptr , ImGui::GetIO().MousePos.x , ImGui::GetIO().MousePos.y );
	}
	else
	{
		m_vecMousePosSave = ImGui::GetIO().MousePos;
	}
}

LRESULT WINAPI CAndromedaGUI::GUI_WndProc( HWND hwnd , UINT uMsg , WPARAM wParam , LPARAM lParam )
{
	if ( uMsg == WM_QUIT || uMsg == WM_CLOSE || uMsg == WM_DESTROY )
	{
		GetDllLauncher()->OnDestroy();
		return true;
	}

	if ( GetAndromedaGUI()->m_bInit )
	{
		if ( uMsg == WM_KEYUP && wParam == VK_INSERT )
			GetAndromedaGUI()->OnReopenGUI();

		if ( GetAndromedaGUI()->IsVisible() && ImGui_ImplWin32_WndProcHandler( hwnd , uMsg , wParam , lParam ) == 0 )
			return true;
	}

	return CallWindowProcA( GetAndromedaGUI()->m_WndProc_o , hwnd , uMsg , wParam , lParam );
}

void CAndromedaGUI::SetModernRedesignStyle()
{
	auto& Style = ImGui::GetStyle();
	auto& Colors = Style.Colors;

	Style.WindowRounding = 10.0f;
	Style.FrameRounding = 5.0f;
	Style.PopupRounding = 8.0f;
	Style.ScrollbarRounding = 12.0f;
	Style.GrabRounding = 5.0f;
	Style.TabRounding = 5.0f;
	Style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
	Style.WindowBorderSize = 1.0f;
	Style.FrameBorderSize = 1.0f;

	Colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
	Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.08f, 0.96f);
	Colors[ImGuiCol_ChildBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
	Colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.10f, 0.94f);
	Colors[ImGuiCol_Border] = ImVec4(0.28f, 0.56f, 1.00f, 0.50f);
	Colors[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.12f, 0.16f, 1.00f);
	Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.18f, 0.18f, 0.24f, 1.00f);
	Colors[ImGuiCol_FrameBgActive] = ImVec4(0.24f, 0.24f, 0.32f, 1.00f);
	Colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
	Colors[ImGuiCol_TitleBgActive] = ImVec4(0.12f, 0.12f, 0.16f, 1.00f);
	Colors[ImGuiCol_CheckMark] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
	Colors[ImGuiCol_SliderGrab] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
	Colors[ImGuiCol_Button] = ImVec4(0.12f, 0.12f, 0.16f, 1.00f);
	Colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
	Colors[ImGuiCol_Header] = ImVec4(0.28f, 0.56f, 1.00f, 0.30f);
	Colors[ImGuiCol_HeaderHovered] = ImVec4(0.28f, 0.56f, 1.00f, 0.50f);
}

auto CAndromedaGUI::UpdateStyle() -> void
{
	ImGui::SetCurrentContext( m_pImGuiContext );
    SetModernRedesignStyle();
}

bool CAndromedaGUI::FreeTypeBuild::PreNewFrame()
{
	if ( !WantRebuild )
		return false;

	ImFontAtlas* atlas = ImGui::GetIO().Fonts;

	for ( int n = 0; n < atlas->Sources.Size; n++ )
		( (ImFontConfig*)&atlas->Sources[n] )->RasterizerMultiply = RasterizerMultiply;

#ifdef IMGUI_ENABLE_FREETYPE
	if ( BuildMode == FontBuildMode::FreeType )
	{
		atlas->FontBuilderIO = ImGuiFreeType::GetBuilderForFreeType();
		atlas->FontBuilderFlags = FreeTypeBuilderFlags;
	}
#endif

	atlas->Build();
	WantRebuild = false;

	return true;
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
