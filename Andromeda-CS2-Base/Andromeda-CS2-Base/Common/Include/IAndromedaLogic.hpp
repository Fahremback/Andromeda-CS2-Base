#pragma once
#include <windows.h>
#include <Common/Common.hpp>
#include <ImGui/imgui.h>
#include <cstdint>

// Forward declarations for SDK types
class IGameEvent;
class CEntityInstance;
namespace CS2 { class CHandle; }
class CCSGOInput;
class CUserCmd;
class IVEngineToClient;
class CGameEntitySystem;
class CSchemaSystem;
class CSource2Client;
class CLocalize;
class CSoundOpSystem;
class IBaseFileSystem;
class CMaterialSystem2;
class IEngineCVar;
class CInputSystem;
class CGlobalVarsBase;

#include <CS2/SDK/Types/CHandle.hpp>

#define ANDROMEDA_INTERFACE_VERSION 0x00000002

struct SettingsStruct {
	struct {
		bool Active = true;
		bool Team = true;
		bool Enemy = true;
		bool OnlyVisible = false;
		bool PlayerBox = true;
		bool BoneESP = true;
		bool BoneESPTeam = true;
		bool BoneESPEnemy = true;
		bool Glow = true;
		bool GlowTeam = true;
		bool GlowEnemy = true;
		int PlayerBoxType = 3;
	} Visual;

	struct {
		int MenuAlpha = 200;
		int MenuStyle = 0;
	} Menu;

	struct {
		ImVec4 PlayerBoxTT = { 255.f / 255.f , 75.f / 255.f , 75.f / 255.f  , 1.f };
		ImVec4 PlayerBoxTT_Visible = { 0 , 1.f , 0.f , 1.f };
		ImVec4 PlayerBoxCT = { 65.f / 255.f , 200 / 255.f , 255.f / 255.f , 1.f };
		ImVec4 PlayerBoxCT_Visible = { 0 , 1.f , 0.f , 1.f };
		ImVec4 BoneESPTT = { 255.f / 255.f , 75.f / 255.f , 75.f / 255.f  , 1.f };
		ImVec4 BoneESPTT_Visible = { 0 , 1.f , 0.f , 1.f };
		ImVec4 BoneESPCT = { 65.f / 255.f , 200 / 255.f , 255.f / 255.f , 1.f };
		ImVec4 BoneESPCT_Visible = { 0 , 1.f , 0.f , 1.f };
		ImVec4 GlowTT = { 255.f / 255.f , 75.f / 255.f , 75.f / 255.f  , 1.f };
		ImVec4 GlowTT_Visible = { 0 , 1.f , 0.f , 1.f };
		ImVec4 GlowCT = { 65.f / 255.f , 200 / 255.f , 255.f / 255.f , 1.f };
		ImVec4 GlowCT_Visible = { 0 , 1.f , 0.f , 1.f };
	} Colors;

	struct {
		bool Enabled = true;
		bool TriggerBot = false;
		float Smooth = 1.0f;
		float FOV = 10.0f;
		int Hitchance = 50;
		int MinDamage = 1;
		bool RCS = true;
		float RCSScaleX = 2.0f;
		float RCSScaleY = 2.0f;
		bool Backtrack = false;
		int BacktrackTicks = 8;
	} Aimbot;

	struct {
		bool Enabled = false;
		int MinDamage = 20;
	} Autowall;

	struct {
		bool Enabled = false;
		bool Desync = false;
		float DesyncStrength = 58.0f;
		int Pitch = 0;
		bool YawJitter = false;
		float YawJitterAmount = 30.0f;
		bool LBYBreaker = false;
		float LBYBreakerSpeed = 40.0f;
	} AntiAim;

	struct {
		bool FakeLag = false;
		int FakeLagTicks = 6;
		bool HideShots = false;
		bool TickbaseShift = false;
	} NetworkExploits;

	struct {
		bool Enabled = false;
		bool BruteForce = false;
		bool AnimationResolve = false;
		bool DesyncCorrect = false;
		bool Extrapolation = false;
	} Resolver;
};

struct SharedState {
    uint32_t interfaceVersion;
    ImGuiContext* imguiContext;
    SettingsStruct* settings;
    
    struct {
        IVEngineToClient* engine;
        CGameEntitySystem* entitySystem;
        CSchemaSystem* schemaSystem;
        CSource2Client* source2Client;
        CLocalize* localize;
        CSoundOpSystem* soundOpSystem;
        IBaseFileSystem* baseFileSystem;
        CMaterialSystem2* materialSystem2;
        IEngineCVar* engineCvar;
        CInputSystem* inputSystem;
        CGlobalVarsBase* globalVars;
    } sdk;
};

class IAndromedaLogic {
public:
    virtual ~IAndromedaLogic() = default;
    
    virtual bool OnInit(SharedState* state) = 0; // Return true if version matches
    virtual void OnFrameStageNotify(int stage) = 0;
    virtual void OnFireEventClientSide(IGameEvent* pEvent) = 0;
    virtual void OnAddEntity(CEntityInstance* pInst, CHandle handle) = 0;
    virtual void OnRemoveEntity(CEntityInstance* pInst, CHandle handle) = 0;
    virtual void OnRender() = 0;
    virtual void OnClientOutput() = 0;
    virtual void OnCreateMove(CCSGOInput* pInput, CUserCmd* pCmd) = 0;
    virtual void OnDestroy() = 0;
};

extern "C" {
    __declspec(dllexport) IAndromedaLogic* GetLogicInstance();
}

typedef IAndromedaLogic* (__cdecl* GetLogicInstanceFn)();
