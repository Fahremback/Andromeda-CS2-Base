#include "CAimbot.hpp"
#include "../../CS2/SDK/Math/Math.hpp"
#include "../../AndromedaClient/Render/CRender.hpp"
#include "../../GameClient/CEntityCache/CEntityCache.hpp"
#include "../../GameClient/CL_Players.hpp"
#include "../../GameClient/CL_VisibleCheck.hpp"
#include "../../GameClient/CL_Bones.hpp"
#include <CS2/SDK/Interface/IEngineToClient.hpp>
#include <immintrin.h>
#include <cmath>
#include <algorithm>
#include <mutex>

thread_local CAimbot::SoAEntityCache CAimbot::ThreadLocalStaging::localCache;
thread_local CAimbot::FireCommand CAimbot::ThreadLocalStaging::pendingCommand;

void CAimbot::Initialize() {
    config = AimbotConfig{};
}

void CAimbot::UpdateEntityCache() {
    ThreadLocalStaging::localCache.Clear();
    
    auto* pEngine = SDK::Interfaces::EngineToClient();
    if (!pEngine || !pEngine->IsInGame())
        return;
    
    auto* pPlayers = GetCL_Players();
    if (!pPlayers) return;

    auto* pLocalController = pPlayers->GetLocalPlayerController();
    if (!pLocalController)
        return;
    
    int localTeam = pLocalController->m_iTeamNum();
    auto* pCache = GetEntityCache();
    if (!pCache) return;

    const auto& cachedVec = pCache->GetCachedEntity();
    if (!cachedVec) return;

    std::scoped_lock lock(pCache->GetLock());
    
    for (const auto& cachedEntity : *cachedVec) {
        auto pEntity = cachedEntity.m_Handle.Get();
        if (!pEntity || cachedEntity.m_Type != CachedEntity_t::PLAYER_CONTROLLER)
            continue;
        
        auto* pController = reinterpret_cast<CCSPlayerController*>(pEntity);
        if (!pController || !pController->m_bPawnIsAlive() || pController->m_iTeamNum() == localTeam)
            continue;
        
        auto* pPawn = pController->m_hPawn().Get<C_CSPlayerPawn>();
        if (!pPawn || !pPawn->IsPlayerPawn())
            continue;

        auto* pBones = GetCL_Bones();
        if (!pBones) continue;

        Vector3 headPos = pBones->GetBonePositionByName(pPawn, "head_0");
        if (headPos.IsZero())
            continue;
        
        bool isVisible = true;
        if (config.visibilityCheck) {
            auto* pVisCheck = GetCL_VisibleCheck();
            isVisible = pVisCheck ? pVisCheck->IsPlayerControllerVisible(pController) : true;
        }
        
        ThreadLocalStaging::localCache.AddEntity(headPos, pController->m_iTeamNum(), (float)pPawn->m_iHealth(), isVisible);
    }
}

__m512 CAimbot::fast_rsqrt14_ps(__m512 v) {
    return _mm512_rsqrt14_ps(v);
}

void CAimbot::Execute(Vector3& viewAngles, bool& shouldShoot) {
    if (!config.enabled) return;

    auto& cache = ThreadLocalStaging::localCache;
    if (cache.entityCount == 0) return;

    auto* pPlayers = GetCL_Players();
    if (!pPlayers) return;

    auto* pLocalPawn = pPlayers->GetLocalPlayerPawn();
    if (!pLocalPawn) return;

    // CORREÇÃO 1: Posição dos Olhos (EyePos) para evitar o 180
    // Origin + ViewOffset = Onde a câmera realmente está
    Vector3 eyePos = pLocalPawn->m_vOldOrigin() + pLocalPawn->m_vecViewOffset();
    
    FireCommand& best = ThreadLocalStaging::pendingCommand;
    best.shouldFire = false;
    best.fov = config.fov;

    for (size_t i = 0; i < cache.entityCount; i++) {
        if (!cache.isActive[i]) continue;
        if (config.visibilityCheck && !cache.isVisible[i]) continue;

        Vector3 targetPos(cache.headX[i], cache.headY[i], cache.headZ[i]);
        ImVec2 screenPos;
        
        if (Math::WorldToScreen(targetPos, screenPos)) {
            ImVec2 screenSize = ImGui::GetIO().DisplaySize;
            if (screenSize.x <= 0 || screenSize.y <= 0) continue;

            ImVec2 center(screenSize.x * 0.5f, screenSize.y * 0.5f);
            
            float dx = screenPos.x - center.x;
            float dy = screenPos.y - center.y;
            float currentFOV = std::sqrt(dx*dx + dy*dy) * 0.1f;

            if (currentFOV < best.fov) {
                best.shouldFire = true;
                best.fov = currentFOV;
                
                // Cálculo de ângulo partindo dos OLHOS
                QAngle qAngle = Math::CalcAngle(eyePos, targetPos);
                best.targetAngle = Vector3(qAngle.m_x, qAngle.m_y, qAngle.m_z);
            }
        }
    }

    if (best.shouldFire) {
        QAngle finalAngle(best.targetAngle.m_x, best.targetAngle.m_y, best.targetAngle.m_z);

        // RCS: usar o último entry do m_aimPunchCache (punch atual)
        if (config.recoilControl) {
            auto& punchCache = pLocalPawn->m_aimPunchCache();
            if (punchCache.Count() > 0) {
                QAngle punch = punchCache[punchCache.Count() - 1];
                finalAngle.m_x -= punch.m_x * 2.0f;
                finalAngle.m_y -= punch.m_y * 2.0f;
            }
        }

        Math::NormalizeAngles(finalAngle);
        Math::ClampAngles(finalAngle);

        if (config.smooth > 1.0f) {
            QAngle current(viewAngles.m_x, viewAngles.m_y, viewAngles.m_z);
            QAngle out;
            Math::SmoothAngles(current, finalAngle, out, config.smooth);
            viewAngles.m_x = out.m_x;
            viewAngles.m_y = out.m_y;
            viewAngles.m_z = out.m_z;
        } else {
            viewAngles.m_x = finalAngle.m_x;
            viewAngles.m_y = finalAngle.m_y;
            viewAngles.m_z = finalAngle.m_z;
        }

        if (best.fov < 1.0f) shouldShoot = true;
    }
}

bool CAimbot::HasAVX512() {
    int cpuInfo[4];
    __cpuidex(cpuInfo, 7, 0);
    return (cpuInfo[1] & (1 << 16)) != 0;
}
