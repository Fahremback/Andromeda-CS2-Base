#include "CAndromedaClient.hpp"

#include <CS2/SDK/SDK.hpp>
#include <CS2/SDK/Interface/IEngineToClient.hpp>
#include <CS2/SDK/Interface/IGameEvent.hpp>
#include <CS2/SDK/Update/CCSGOInput.hpp>

#include <Common/LinearArena.hpp>

#include <AndromedaClient/GUI/CAndromedaMenu.hpp>
#include <AndromedaClient/CAndromedaGUI.hpp>
#include <AndromedaClient/Fonts/CFontManager.hpp>
#include <AndromedaClient/Render/CRenderStackSystem.hpp>
#include <AndromedaClient/Features/CVisual/CVisual.hpp>

#include <GameClient/CEntityCache/CEntityCache.hpp>

static CAndromedaClient g_CAndromedaClient{};

auto CAndromedaClient::OnInit() -> void
{
	m_Initialized.store( true , std::memory_order_release );
}

auto CAndromedaClient::OnFrameStageNotify( int FrameStage ) -> void
{

}

auto CAndromedaClient::OnFireEventClientSide( IGameEvent* pGameEvent ) -> void
{

}

auto CAndromedaClient::OnAddEntity( CEntityInstance* pInst , CHandle handle ) -> void
{
	GetEntityCache()->OnAddEntity( pInst , handle );
}

auto CAndromedaClient::OnRemoveEntity( CEntityInstance* pInst , CHandle handle ) -> void
{
	GetEntityCache()->OnRemoveEntity( pInst , handle );
}

auto CAndromedaClient::OnRender() -> void
{
	if ( !m_Initialized.load( std::memory_order_acquire ) )
		return;

	if ( GetAndromedaGUI()->IsVisible() )
		GetAndromedaMenu()->OnRenderMenu();

	GetFontManager()->FirstInitFonts();
	GetFontManager()->m_VerdanaFont.DrawString( 1 , 1 , ImColor( 255 , 255 , 0 ) , FW1_LEFT , XorStr( CHEAT_NAME ) );

	if ( m_InGame.load( std::memory_order_acquire ) )
	{
		GetRenderStackSystem()->OnRenderStack();
	}
}

auto CAndromedaClient::OnClientOutput() -> void
{
	if ( m_InGame.load( std::memory_order_acquire ) )
	{
		GetVisual()->OnClientOutput();
	}
}

auto CAndromedaClient::OnCreateMove( CCSGOInput* pInput , CUserCmd* pUserCmd ) -> void
{
	m_InGame.store( SDK::Interfaces::EngineToClient()->IsInGame() , std::memory_order_release );
	GetLinearArena()->Reset();
	GetVisual()->OnCreateMove();
}

auto GetAndromedaClient() -> CAndromedaClient*
{
	return &g_CAndromedaClient;
}
