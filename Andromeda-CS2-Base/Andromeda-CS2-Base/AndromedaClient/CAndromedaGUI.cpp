#include "CAndromedaGUI.hpp"

#include <ShlObj_core.h>

#include <ImGui/imgui_impl_win32.h>
#include <ImGui/imgui_impl_dx11.h>

#include <DllLauncher.hpp>
#include <Common/Helpers/StringHelper.hpp>

#include <CS2/SDK/SDK.hpp>
#include <AndromedaClient/CAndromedaClient.hpp>

static CAndromedaGUI g_AndromedaGUI{};

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

auto CAndromedaGUI::OnPresent( IDXGISwapChain* pSwapChain ) -> void
{
	if ( !m_bInit )
	{
		OnInit( pSwapChain );
		return;
	}

	// Prepare ImGui frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// Create Render Target if needed
	if ( !m_pRenderTargetView )
	{
		ID3D11Texture2D* pBackBuffer = nullptr;
		if ( SUCCEEDED( pSwapChain->GetBuffer( 0 , __uuidof( ID3D11Texture2D ) , (void**)&pBackBuffer ) ) )
		{
			m_pDevice->CreateRenderTargetView( pBackBuffer , nullptr , &m_pRenderTargetView );
			pBackBuffer->Release();
		}
	}

	// Important: Let the Client (Bridge) call the Logic DLL to render its UI
	if (m_pRenderTargetView) 
	{
		GetAndromedaClient()->OnRender();
	}

	// Finish ImGui frame
	ImGui::Render();

	if (m_pRenderTargetView)
	{
		m_pDeviceContext->OMSetRenderTargets( 1 , &m_pRenderTargetView , nullptr );
		ImGui_ImplDX11_RenderDrawData( ImGui::GetDrawData() );
	}
}

auto CAndromedaGUI::OnResizeBuffers( IDXGISwapChain* pSwapChain ) -> void
{
	ClearRenderTargetView();
}

auto CAndromedaGUI::OnRender( IDXGISwapChain* pSwapChain ) -> void
{
	// This is called by the old system, we handle everything in OnPresent now
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
