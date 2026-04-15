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
	Andromeda::Features::CAimbot::Initialize();
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
	Vector3 viewAngles(pUserCmd->viewangles.x, pUserCmd->viewangles.y, pUserCmd->viewangles.z);
	bool shouldShoot = false;
	
	Andromeda::Features::CAimbot::Execute(viewAngles, shouldShoot);
	
	// Aplicar ângulos calculados
	pUserCmd->viewangles.x = viewAngles.x;
	pUserCmd->viewangles.y = viewAngles.y;
	pUserCmd->viewangles.z = viewAngles.z;
	
	// Disparar se necessário
	if (shouldShoot)
	{
		pUserCmd->buttons |= IN_ATTACK;
	}
}

auto GetAndromedaClient() -> CAndromedaClient*
{
	return &g_CAndromedaClient;
}
