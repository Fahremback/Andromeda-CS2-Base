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
#include <AndromedaClient/Features/Targeting/CTargetingSystem.hpp>
#include <AndromedaClient/Features/Ballistics/CBallisticsSystem.hpp>
#include <AndromedaClient/Performance/LatencyPipeline.hpp>
#include <AndromedaClient/Settings/Settings.hpp>

#include <GameClient/CEntityCache/CEntityCache.hpp>
#include <chrono>

static CAndromedaClient g_CAndromedaClient{};

auto CAndromedaClient::OnInit() -> void
{
	GetLatencyPipeline()->Start( []( const CLatencyPipeline::InputEvent_t& Event )
	{
		( void )Event;
	} );

	GetBallisticsSystem()->SetMaterialTable(
	{
		{ CBallisticsSystem::EMaterial::AIR , 0.f } ,
		{ CBallisticsSystem::EMaterial::WOOD , 0.8f } ,
		{ CBallisticsSystem::EMaterial::CONCRETE , 1.3f } ,
		{ CBallisticsSystem::EMaterial::METAL , 1.8f } ,
		{ CBallisticsSystem::EMaterial::GLASS , 0.4f }
	} );

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
		GetLatencyPipeline()->BeginStage( CStageProfiler::EStage::RENDER_SUBMIT );
		GetTargetingSystem()->OnRenderDebug();
		GetRenderStackSystem()->OnRenderStack();
		GetLatencyPipeline()->EndStage( CStageProfiler::EStage::RENDER_SUBMIT );

		const auto SimStats = GetLatencyPipeline()->GetProfiler().Snapshot( CStageProfiler::EStage::SIMULATION );
		const auto RenderStats = GetLatencyPipeline()->GetProfiler().Snapshot( CStageProfiler::EStage::RENDER_SUBMIT );

		GetFontManager()->m_VerdanaFont.DrawString( 8 , 56 , ImColor( 180 , 255 , 180 ) , FW1_LEFT ,
			"SIM p99=%.1fus p999=%.1fus | RENDER p99=%.1fus p999=%.1fus | SIMD=%s" ,
			SimStats.P99Us , SimStats.P999Us , RenderStats.P99Us , RenderStats.P999Us , GetLatencyPipeline()->GetBestSIMDPathName() );
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
	static uint64_t TickId = 0;
	GetLatencyPipeline()->EnqueueInput( { ++TickId , 0 , 0 , static_cast<uint64_t>( std::chrono::high_resolution_clock::now().time_since_epoch().count() ) } );
	GetLatencyPipeline()->BeginStage( CStageProfiler::EStage::SIMULATION );

	m_InGame.store( SDK::Interfaces::EngineToClient()->IsInGame() , std::memory_order_release );
	GetBallisticsSystem()->SetSettings( {
		Settings::Ballistics::BaseDamage ,
		Settings::Ballistics::MinDamageThreshold ,
		Settings::Ballistics::MaxPenetrations
	} );
	GetLinearArena()->Reset();
	GetVisual()->OnCreateMove();
	GetTargetingSystem()->OnCreateMove();
	GetLatencyPipeline()->EndStage( CStageProfiler::EStage::SIMULATION );
}

auto GetAndromedaClient() -> CAndromedaClient*
{
	return &g_CAndromedaClient;
}
