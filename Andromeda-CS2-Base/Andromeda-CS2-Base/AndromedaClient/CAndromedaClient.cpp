#include "CAndromedaClient.hpp"

#include <CS2/SDK/SDK.hpp>
#include <CS2/SDK/Interface/IEngineToClient.hpp>
#include <CS2/SDK/Interface/IGameEvent.hpp>
#include <CS2/SDK/Update/CCSGOInput.hpp>

#include <AndromedaClient/GUI/CAndromedaMenu.hpp>
#include <AndromedaClient/CAndromedaGUI.hpp>
#include <AndromedaClient/Fonts/CFontManager.hpp>
#include <AndromedaClient/Render/CRenderStackSystem.hpp>
#include <AndromedaClient/Features/CVisual/CVisual.hpp>
#include <AndromedaClient/Features/CAimbot.hpp>

#include <GameClient/CEntityCache/CEntityCache.hpp>

static CAndromedaClient g_CAndromedaClient{};

auto CAndromedaClient::OnInit() -> void
{
	// Inicializar Aimbot ultra-otimizado
	CAimbot::Initialize();
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
	if ( GetAndromedaGUI()->IsVisible() )
		GetAndromedaMenu()->OnRenderMenu();

	GetFontManager()->FirstInitFonts();
	GetFontManager()->m_VerdanaFont.DrawString( 1 , 1 , ImColor( 255 , 255 , 0 ) , FW1_LEFT , XorStr( CHEAT_NAME ) );

	if ( SDK::Interfaces::EngineToClient()->IsInGame() )
	{
		GetRenderStackSystem()->OnRenderStack();
	}
}

auto CAndromedaClient::OnClientOutput() -> void
{
	if ( SDK::Interfaces::EngineToClient()->IsInGame() )
	{
		GetVisual()->OnClientOutput();
	}
}

auto CAndromedaClient::OnCreateMove( CCSGOInput* pInput , CUserCmd* pUserCmd ) -> void
{
	GetVisual()->OnCreateMove();
	
	// Executar Aimbot ultra-otimizado
	// Primeiro, atualizar o cache de entidades
	CAimbot::UpdateEntityCache();
	
	// Acessar ângulos através da estrutura protobuf do CS2
	Vector3 viewAngles(
		pUserCmd->cmd.base().viewangles().x(),
		pUserCmd->cmd.base().viewangles().y(),
		pUserCmd->cmd.base().viewangles().z()
	);
	bool shouldShoot = false;
	
	CAimbot::Execute(viewAngles, shouldShoot);
	
	// Aplicar ângulos calculados de volta ao protobuf
	pUserCmd->cmd.mutable_base()->mutable_viewangles()->set_x(viewAngles.m_x);
	pUserCmd->cmd.mutable_base()->mutable_viewangles()->set_y(viewAngles.m_y);
	pUserCmd->cmd.mutable_base()->mutable_viewangles()->set_z(viewAngles.m_z);
	
	// Disparar se necessário - usar button_states
	if (shouldShoot)
	{
		pUserCmd->button_states.buttonstate1 |= IN_ATTACK;
	}
}

auto GetAndromedaClient() -> CAndromedaClient*
{
	return &g_CAndromedaClient;
}
