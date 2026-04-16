#pragma once

#include <Common/Common.hpp>

#include <CS2/SDK/Types/CHandle.hpp>
#include <CS2/SDK/Math/Vector3.hpp>

class IGameEvent;
class CEntityInstance;
class CCSGOInput;
class CUserCmd;

#include <Common/Include/IAndromedaLogic.hpp>
#include <atomic>
#include <filesystem>
#include <string>
#include <memory>

// Forward declaration
class CModuleManager;

class CAndromedaClient final : public IAndromedaLogic
{
public:
	auto OnInit(SharedState* state) -> bool override;
	auto OnFrameStageNotify( int FrameStage ) -> void override;
	auto OnFireEventClientSide( IGameEvent* pGameEvent ) -> void override;
	auto OnAddEntity( CEntityInstance* pInst , CHandle handle ) -> void override;
	auto OnRemoveEntity( CEntityInstance* pInst , CHandle handle ) -> void override;
	auto OnRender() -> void override;
	auto OnClientOutput() -> void override;
	auto OnCreateMove( CCSGOInput* pInput , CUserCmd* pUserCmd ) -> void override;
	auto OnDestroy() -> void override;

public:
	// Legacy hot-reload support (still works for Andromeda-Logic.dll)
	void ReloadLogic();
	bool IsLogicLoaded() const { return m_pLogic != nullptr; }
	SettingsStruct* GetSettings() { return &m_Settings; }
	
	// New Module Manager integration
	CModuleManager* GetModuleManager() const { return m_pModuleManager; }
	void SetSharedState(SharedState* state) { m_SharedStatePtr = state; }

private:
	IAndromedaLogic* m_pLogic = nullptr;
	HMODULE m_hLogicModule = nullptr;
	std::filesystem::file_time_type m_LastLoadTime;
	std::chrono::steady_clock::time_point m_LastFileCheckTime;
	std::atomic<bool> m_bIsReloading = false;
	std::uint64_t m_ReloadGeneration = 0;
	std::string m_CurrentSwapPath;
	SharedState m_SharedState;
	SharedState* m_SharedStatePtr = nullptr; // Pointer for module manager
	SettingsStruct m_Settings;
	CModuleManager* m_pModuleManager = nullptr;
};

auto GetAndromedaClient() -> CAndromedaClient*;
