#pragma once

#include <Common/Common.hpp>

#include <CS2/SDK/Types/CHandle.hpp>
#include <CS2/SDK/Math/Vector3.hpp>

class IGameEvent;
class CEntityInstance;
class CCSGOInput;
class CUserCmd;

#include <Common/Include/IAndromedaLogic.hpp>

class CAndromedaClient final : public IAndromedaLogic
{
public:
	virtual bool OnInit(SharedState* state) override;
	virtual void OnFrameStageNotify( int FrameStage ) override;
	virtual void OnFireEventClientSide( IGameEvent* pGameEvent ) override;
	virtual void OnAddEntity( CEntityInstance* pInst , CHandle handle ) override;
	virtual void OnRemoveEntity( CEntityInstance* pInst , CHandle handle ) override;
	virtual void OnRender() override;
	virtual void OnClientOutput() override;
	virtual void OnCreateMove( CCSGOInput* pInput , CUserCmd* pUserCmd ) override;
	virtual void OnDestroy() override;

public:
	SettingsStruct* GetSettings() { 
		static SettingsStruct dummy;
		return m_SharedState ? m_SharedState->settings : &dummy; 
	}

private:
	SharedState* m_SharedState = nullptr;
};

auto GetAndromedaClient() -> CAndromedaClient*;
