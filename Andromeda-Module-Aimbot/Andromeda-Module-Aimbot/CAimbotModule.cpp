#include "CAimbotModule.hpp"
#include <CS2/SDK/SDK.hpp>
#include <Common/CrashLog.hpp>

CAimbotModule::CAimbotModule()
{
    m_State = ModuleState::Unloaded;
}

const ModuleInfo& CAimbotModule::GetModuleInfo() const
{
    return m_Info;
}

bool CAimbotModule::OnInit(SharedState* state)
{
    if (!state || state->interfaceVersion != ANDROMEDA_MODULE_INTERFACE_VERSION)
    {
        return false;
    }
    
    m_SharedState = state;
    m_State = ModuleState::Loaded;
    
    DEV_LOG("[AimbotModule] Initialized successfully\n");
    
    return true;
}

void CAimbotModule::OnDestroy()
{
    m_State = ModuleState::Unloaded;
    m_SharedState = nullptr;
    
    DEV_LOG("[AimbotModule] Destroyed\n");
}

void CAimbotModule::OnFrameStageNotify(int stage)
{
    if (!m_Enabled || !m_SharedState)
        return;
    
    // Frame stage processing if needed
}

void CAimbotModule::OnFireEventClientSide(IGameEvent* pEvent)
{
    if (!m_Enabled || !pEvent)
        return;
    
    // Event handling if needed
}

void CAimbotModule::OnAddEntity(CEntityInstance* pInst, CS2::CHandle handle)
{
    if (!m_Enabled)
        return;
    
    // Entity tracking if needed
}

void CAimbotModule::OnRemoveEntity(CEntityInstance* pInst, CS2::CHandle handle)
{
    if (!m_Enabled)
        return;
    
    // Entity cleanup if needed
}

void CAimbotModule::OnRender()
{
    if (!m_Enabled || !m_SharedState)
        return;
    
    // Render ESP or debug info if needed
    // ImGui can be accessed via m_SharedState->imguiContext
}

void CAimbotModule::OnClientOutput()
{
    if (!m_Enabled)
        return;
    
    // Client output processing if needed
}

void CAimbotModule::OnCreateMove(CCSGOInput* pInput, CUserCmd* pCmd)
{
    if (!m_Enabled || !pInput || !pCmd)
        return;
    
    RunAimbot(pInput, pCmd);
}

auto CAimbotModule::RunAimbot(CCSGOInput* pInput, CUserCmd* pCmd) -> void
{
    if (!m_SharedState || !m_SharedState->sdk.globalVars)
        return;
    
    // Find target
    void* pTarget = FindTarget();
    if (!pTarget)
        return;
    
    // Get local player view angles
    Vector3 currentAngles(
        pCmd->cmd.base().viewangles().x(),
        pCmd->cmd.base().viewangles().y(),
        pCmd->cmd.base().viewangles().z()
    );
    
    // Calculate target angle (placeholder - implement proper bone calculation)
    Vector3 targetAngles = currentAngles;
    
    // Apply smooth
    if (m_Smooth > 1.0f)
    {
        targetAngles = SmoothAngle(currentAngles, targetAngles, m_Smooth);
    }
    
    // Set new angles
    pCmd->cmd.mutable_base()->mutable_viewangles()->set_x(targetAngles.m_x);
    pCmd->cmd.mutable_base()->mutable_viewangles()->set_y(targetAngles.m_y);
}

auto CAimbotModule::FindTarget() -> void*
{
    // Placeholder: Implement entity iteration and target selection
    // Should use m_SharedState->sdk.entitySystem to iterate entities
    return nullptr;
}

auto CAimbotModule::CalculateAngle(Vector3 src, Vector3 dst) -> Vector3
{
    Vector3 delta = dst - src;
    Vector3 angles;
    
    angles.m_x = -atan2f(delta.m_z, sqrtf(delta.m_x * delta.m_x + delta.m_y * delta.m_y)) * (180.0f / 3.14159265358979323846f);
    angles.m_y = atan2f(delta.m_y, delta.m_x) * (180.0f / 3.14159265358979323846f);
    angles.m_z = 0.0f;
    
    return angles;
}

auto CAimbotModule::SmoothAngle(Vector3 current, Vector3 target, float smooth) -> Vector3
{
    Vector3 delta = target - current;
    
    delta.m_x /= smooth;
    delta.m_y /= smooth;
    delta.m_z = 0.0f;
    
    return current + delta;
}
