#include "CAndromedaClient.hpp"

#include <CS2/SDK/SDK.hpp>
#include <CS2/SDK/Interface/IEngineToClient.hpp>
#include <CS2/SDK/Interface/IGameEvent.hpp>
#include <CS2/SDK/Update/CCSGOInput.hpp>

#include <AndromedaClient/GUI/CAndromedaMenu.hpp>
#include <AndromedaClient/Features/CVisual/CVisual.hpp>
#include <AndromedaClient/Features/CPhysicsAligner.hpp>
#include <AndromedaClient/Settings/Settings.hpp>

static CAndromedaClient g_CAndromedaClient{};

bool CAndromedaClient::OnInit(SharedState* state)
{
	if (!state || state->interfaceVersion != ANDROMEDA_INTERFACE_VERSION) {
		return false;
	}

	m_SharedState = state;
	
	// Link ImGui context passed from Bridge
	if (state->imguiContext) {
		ImGui::SetCurrentContext(state->imguiContext);
	}
	
	// Link SDK pointers
	SDK::Interfaces::SetSharedState(state);
	
	CPhysicsAligner::Initialize();
	
	return true;
}

void CAndromedaClient::OnDestroy()
{
	CPhysicsAligner::OnDestroy();
}

auto CAndromedaClient::OnFrameStageNotify( int FrameStage ) -> void
{
	auto* pEngine = SDK::Interfaces::EngineToClient();
	if ( !pEngine || !pEngine->IsInGame() )
		return;

	if ( FrameStage == 4 ) // FRAME_RENDER_START
	{
		CPhysicsAligner::RequestAsyncProcess();
	}
}

auto CAndromedaClient::OnFireEventClientSide( IGameEvent* pGameEvent ) -> void
{
}

auto CAndromedaClient::OnAddEntity( CEntityInstance* pInst , CHandle handle ) -> void
{
}

auto CAndromedaClient::OnRemoveEntity( CEntityInstance* pInst , CHandle handle ) -> void
{
}

auto CAndromedaClient::OnRender() -> void
{
	// Render ESP
	GetVisual()->OnRender();
	
	// Render Menu
	GetAndromedaMenu()->OnRenderMenu();
}

auto CAndromedaClient::OnClientOutput() -> void
{
}

auto CAndromedaClient::OnCreateMove( CCSGOInput* pInput , CUserCmd* pUserCmd ) -> void
{
	if ( !pInput || !pUserCmd )
		return;

	CPhysicsAligner::AimbotResult result;
	if ( CPhysicsAligner::ConsumeResult( result ) )
	{
		if ( result.shouldFire )
		{
			pUserCmd->cmd.mutable_base()->mutable_viewangles()->set_x( result.finalAngle.m_x );
			pUserCmd->cmd.mutable_base()->mutable_viewangles()->set_y( result.finalAngle.m_y );
		}
	}

	CPhysicsAligner::ioState.collisionAngles = Vector3(
		pUserCmd->cmd.base().viewangles().x(),
		pUserCmd->cmd.base().viewangles().y(),
		pUserCmd->cmd.base().viewangles().z()
	);
	CPhysicsAligner::ioState.renderAngles = CPhysicsAligner::ioState.collisionAngles;
}

extern "C" {
	__declspec(dllexport) IAndromedaLogic* GetLogicInstance()
	{
		return GetAndromedaClient();
	}
}

auto GetAndromedaClient() -> CAndromedaClient*
{
	return &g_CAndromedaClient;
}
