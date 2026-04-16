#pragma once

#include <Common/Include/IAndromedaModule.hpp>
#include <CS2/SDK/Math/Vector3.hpp>
#include <atomic>

/**
 * @brief Módulo de Autowall para Andromeda-CS2
 * 
 * Módulo independente que calcula dano através de paredes.
 * Pode ser carregado/descarregado dinamicamente.
 */
class CAutowallModule final : public IAndromedaModule
{
public:
    CAutowallModule();
    ~CAutowallModule() override = default;

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

    // Public API for other modules
    auto CanHitTarget(Vector3 start, Vector3 end, int minDamage) -> bool;
    auto GetDamageThroughWall(Vector3 start, Vector3 end) -> float;

private:
    auto TraceLine(Vector3 start, Vector3 end, void** pEntity) -> bool;
    auto CalculateDamage(void* pEntity, Vector3 hitPos) -> float;

private:
    ModuleInfo m_Info;
    SharedState* m_SharedState = nullptr;
    ModuleState m_State = ModuleState::Unloaded;
    std::atomic<bool> m_Enabled = true;
    
    // Autowall settings
    int m_MinDamage = 20;
    bool m_ShowDebugOverlay = false;
};

ANDROMEDA_MODULE_BEGIN(CAutowallModule) {}
ANDROMEDA_MODULE_END()

DECLARE_MODULE_INFO(
    "Andromeda Autowall",
    "Andromeda Team",
    "1.0.0",
    "Módulo de cálculo de dano através de paredes"
)
