#include "CAndromedaClient.hpp"
#include <AndromedaClient/CAndromedaGUI.hpp>
#include <DllLauncher.hpp>
#include <CS2/SDK/SDK.hpp>
#include <chrono>
#include <thread>
#include <fstream>

static CAndromedaClient g_CAndromedaClient{};

auto CAndromedaClient::OnInit(SharedState* state) -> bool
{
	m_SharedState.interfaceVersion = ANDROMEDA_INTERFACE_VERSION;
	m_SharedState.imguiContext = GetAndromedaGUI()->GetImGuiContext();
	m_SharedState.settings = &m_Settings;
	
	// SDK Pointers initialization
	m_SharedState.sdk.engine = SDK::Interfaces::EngineToClient();
	m_SharedState.sdk.entitySystem = SDK::Interfaces::GameEntitySystem();
	m_SharedState.sdk.schemaSystem = SDK::Interfaces::SchemaSystem();
	m_SharedState.sdk.source2Client = SDK::Interfaces::Source2Client();
	m_SharedState.sdk.localize = SDK::Interfaces::Localize();
	m_SharedState.sdk.soundOpSystem = SDK::Interfaces::SoundOpSystem();
	m_SharedState.sdk.baseFileSystem = SDK::Interfaces::BaseFileSystem();
	m_SharedState.sdk.materialSystem2 = SDK::Interfaces::MaterialSystem2();
	m_SharedState.sdk.engineCvar = SDK::Interfaces::EngineCvar();
	m_SharedState.sdk.inputSystem = SDK::Interfaces::InputSystem();
	m_SharedState.sdk.globalVars = SDK::Pointers::GlobalVarsBase();

	m_LastFileCheckTime = std::chrono::steady_clock::now();
	ReloadLogic();

	return true;
}

void CAndromedaClient::ReloadLogic()
{
	if (m_bIsReloading) return;
	m_bIsReloading = true;

	std::string logicPath = GetDllDir() + "Andromeda-Logic.dll";
	
	if (!std::filesystem::exists(logicPath)) {
		m_bIsReloading = false;
		return;
	}

	// NEW: Improved lock detection. 
	// We try to rename the source file to itself. If we can't, it's definitely locked.
	bool isLocked = true;
	for (int i = 0; i < 10; ++i) {
		std::error_code ec;
		std::filesystem::rename(logicPath, logicPath, ec);
		if (!ec) {
			isLocked = false;
			break;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	if (isLocked) {
		m_bIsReloading = false;
		return;
	}

	if (m_pLogic) {
		m_pLogic->OnDestroy();
		m_pLogic = nullptr;
	}

	if (m_hLogicModule) {
		FreeLibrary(m_hLogicModule);
		m_hLogicModule = nullptr;
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	m_ReloadGeneration++;
	// Use a unique name for every single reload to avoid destination locks
	std::string swapPath = GetDllDir() + "Andromeda-Logic-Swap-" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count() % 10000) + ".dll";

	try {
		std::filesystem::copy_file(logicPath, swapPath, std::filesystem::copy_options::overwrite_existing);
		
		m_hLogicModule = LoadLibraryA(swapPath.c_str());
		if (m_hLogicModule) {
			auto pGetLogicInstance = (GetLogicInstanceFn)GetProcAddress(m_hLogicModule, "GetLogicInstance");
			if (pGetLogicInstance) {
				m_pLogic = pGetLogicInstance();
				if (m_pLogic && m_pLogic->OnInit(&m_SharedState)) {
					m_LastLoadTime = std::filesystem::last_write_time(logicPath);
					m_CurrentSwapPath = swapPath;
					DEV_LOG("[bridge] Logic hot-reloaded successfully. Gen: %d\n", (int)m_ReloadGeneration);
				}
			}
		}
		
		// Cleanup old swap files
		for (auto& p : std::filesystem::directory_iterator(GetDllDir())) {
			if (p.path().extension() == ".dll" && p.path().filename().string().find("Andromeda-Logic-Swap-") != std::string::npos) {
				if (p.path().string() != swapPath) {
					std::error_code ec;
					std::filesystem::remove(p.path(), ec);
				}
			}
		}
	}
	catch (...) {}

	m_bIsReloading = false;
}

auto CAndromedaClient::OnFrameStageNotify(int FrameStage) -> void
{
	if (m_bIsReloading || !m_pLogic) return;
	m_pLogic->OnFrameStageNotify(FrameStage);
}

auto CAndromedaClient::OnFireEventClientSide(IGameEvent* pGameEvent) -> void
{
	if (m_bIsReloading || !m_pLogic) return;
	m_pLogic->OnFireEventClientSide(pGameEvent);
}

auto CAndromedaClient::OnAddEntity(CEntityInstance* pInst, CHandle handle) -> void
{
	if (m_bIsReloading || !m_pLogic) return;
	m_pLogic->OnAddEntity(pInst, handle);
}

auto CAndromedaClient::OnRemoveEntity(CEntityInstance* pInst, CHandle handle) -> void
{
	if (m_bIsReloading || !m_pLogic) return;
	m_pLogic->OnRemoveEntity(pInst, handle);
}

auto CAndromedaClient::OnRender() -> void
{
	auto now = std::chrono::steady_clock::now();
	if (std::chrono::duration_cast<std::chrono::milliseconds>(now - m_LastFileCheckTime).count() > 1000) {
		m_LastFileCheckTime = now;
		
		std::string logicPath = GetDllDir() + "Andromeda-Logic.dll";
		std::error_code ec;
		if (std::filesystem::exists(logicPath, ec)) {
			auto lastWrite = std::filesystem::last_write_time(logicPath, ec);
			if (!ec && lastWrite != m_LastLoadTime) {
				ReloadLogic();
			}
		}
	}

	if (m_bIsReloading || !m_pLogic) return;
	
	m_SharedState.imguiContext = GetAndromedaGUI()->GetImGuiContext();
	m_pLogic->OnRender();
}

auto CAndromedaClient::OnClientOutput() -> void
{
	if (m_bIsReloading || !m_pLogic) return;
	m_pLogic->OnClientOutput();
}

auto CAndromedaClient::OnCreateMove(CCSGOInput* pInput, CUserCmd* pUserCmd) -> void
{
	if (m_bIsReloading || !m_pLogic) return;
	m_pLogic->OnCreateMove(pInput, pUserCmd);
}

auto CAndromedaClient::OnDestroy() -> void
{
	if (m_pLogic) {
		m_pLogic->OnDestroy();
		m_pLogic = nullptr;
	}
	if (m_hLogicModule) {
		FreeLibrary(m_hLogicModule);
		m_hLogicModule = nullptr;
	}
}

auto GetAndromedaClient() -> CAndromedaClient*
{
	return &g_CAndromedaClient;
}
