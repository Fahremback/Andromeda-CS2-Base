#pragma once

#include <Common/Include/IAndromedaModule.hpp>
#include <CS2/SDK/Math/Vector3.hpp>
#include <atomic>

/**
 * @brief Módulo de Aimbot para Andromeda-CS2
 * 
 * Este é um exemplo de módulo independente que pode ser carregado/descarregado
 * dinamicamente sem reiniciar o jogo.
 * 
 * Uso:
 *   1. Compile como DLL separada (Andromeda-Module-Aimbot.dll)
 *   2. Coloque na pasta "modules" do diretório do cheat
 *   3. O módulo será carregado automaticamente na inicialização
 *   4. Ou use: GetModuleManager()->LoadModule("modules/aimbot.dll")
 */
class CAimbotModule final : public IAndromedaModule
{
public:
    CAimbotModule();
    ~CAimbotModule() override = default;

    // IAndromedaModule interface
    const ModuleInfo& GetModuleInfo() const override;
    bool OnInit(SharedState* state) override;
    void OnFrameStageNotify(int stage) override;
    void OnFireEventClientSide(IGameEvent* pEvent) override;
    void OnAddEntity(CEntityInstance* pInst, CS2::CHandle handle) override;
    void OnRemoveEntity(CEntityInstance* pInst, CS2::CHandle handle) override;
    void OnRender() override;
    void OnClientOutput() override;
    void OnCreateMove(CCSGOInput* pInput, CUserCmd* pCmd) override;
    void OnDestroy() override;
    
    // Module state management
    ModuleState GetState() const override { return m_State; }
    void SetEnabled(bool enabled) override { m_Enabled = enabled; }
    bool IsEnabled() const override { return m_Enabled; }

private:
    auto RunAimbot(CCSGOInput* pInput, CUserCmd* pCmd) -> void;
    auto FindTarget() -> void*;
    auto CalculateAngle(Vector3 src, Vector3 dst) -> Vector3;
    auto SmoothAngle(Vector3 current, Vector3 target, float smooth) -> Vector3;

private:
    ModuleInfo m_Info;
    SharedState* m_SharedState = nullptr;
    ModuleState m_State = ModuleState::Unloaded;
    std::atomic<bool> m_Enabled = true;
    
    // Aimbot settings (could be loaded from config)
    float m_FOV = 10.0f;
    float m_Smooth = 1.0f;
    int m_BoneTarget = 6; // Head
    bool m_AutoFire = false;
};

// Macro para registrar o módulo
ANDROMEDA_MODULE_BEGIN(CAimbotModule)
{
    // Constructor initialization if needed
}
ANDROMEDA_MODULE_END()

// Declaração das informações do módulo
DECLARE_MODULE_INFO(
    "Andromeda Aimbot",
    "Andromeda Team",
    "1.0.0",
    "Módulo de aimbot com Silent Aim e Legit Bot"
)
