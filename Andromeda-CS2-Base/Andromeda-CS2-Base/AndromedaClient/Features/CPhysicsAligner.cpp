
#include "CPhysicsAligner.hpp"
#include "../../CS2/SDK/Math/Math.hpp"
#include "../../AndromedaClient/Render/CRender.hpp"
#include "../../GameClient/CEntityCache/CEntityCache.hpp"
#include "../../GameClient/CL_Players.hpp"
#include "../../GameClient/CL_VisibleCheck.hpp"
#include "../../GameClient/CL_Bones.hpp"
#include <CS2/SDK/Interface/IEngineToClient.hpp>
#include <CS2/Hook/Hook_GetMatricesForView.hpp>
#include <immintrin.h>
#include <cmath>
#include <algorithm>
#include <mutex>

thread_local CPhysicsAligner::SoAEntityCache CPhysicsAligner::ThreadLocalStaging::localCache;
thread_local CPhysicsAligner::FireCommand CPhysicsAligner::ThreadLocalStaging::pendingCommand;
thread_local CPhysicsAligner::ArenaAllocator CPhysicsAligner::ThreadLocalStaging::localArena;

void CPhysicsAligner::Initialize() {
    config = BallSimulationParams{};
}

void CPhysicsAligner::ProjectCoordinatesToGrid_AVX512(SoAEntityCache& cache, const VMatrix& viewMatrix) {
    if (cache.entityCount == 0) return;

    __m512 m0 = _mm512_set1_ps(viewMatrix.m_data[0][0]); __m512 m1 = _mm512_set1_ps(viewMatrix.m_data[0][1]);
    __m512 m2 = _mm512_set1_ps(viewMatrix.m_data[0][2]); __m512 m3 = _mm512_set1_ps(viewMatrix.m_data[0][3]);
    __m512 m4 = _mm512_set1_ps(viewMatrix.m_data[1][0]); __m512 m5 = _mm512_set1_ps(viewMatrix.m_data[1][1]);
    __m512 m6 = _mm512_set1_ps(viewMatrix.m_data[1][2]); __m512 m7 = _mm512_set1_ps(viewMatrix.m_data[1][3]);
    __m512 m12 = _mm512_set1_ps(viewMatrix.m_data[3][0]); __m512 m13 = _mm512_set1_ps(viewMatrix.m_data[3][1]);
    __m512 m14 = _mm512_set1_ps(viewMatrix.m_data[3][2]); __m512 m15 = _mm512_set1_ps(viewMatrix.m_data[3][3]);

    const ImVec2 screen = ImGui::GetIO().DisplaySize;
    if (screen.x <= 0 || screen.y <= 0) return;

    __m512 centerX = _mm512_set1_ps(screen.x * 0.5f);
    __m512 centerY = _mm512_set1_ps(screen.y * 0.5f);

    for (size_t i = 0; i < cache.entityCount; i += 16) {
        __m512 x = _mm512_loadu_ps(&cache.headX[i]);
        __m512 y = _mm512_loadu_ps(&cache.headY[i]);
        __m512 z = _mm512_loadu_ps(&cache.headZ[i]);

        __m512 w = _mm512_fmadd_ps(x, m12, _mm512_fmadd_ps(y, m13, _mm512_fmadd_ps(z, m14, m15)));
        __mmask16 frontMask = _mm512_cmp_ps_mask(w, _mm512_set1_ps(0.01f), _CMP_GT_OS);
        
        if (frontMask == 0) {
            for(int j=0; j<16; j++) cache.onScreen[i+j] = false;
            continue;
        }

        __m512 invW = _mm512_div_ps(_mm512_set1_ps(1.0f), w);
        __m512 projX = _mm512_fmadd_ps(x, m0, _mm512_fmadd_ps(y, m1, _mm512_fmadd_ps(z, m2, m3)));
        projX = _mm512_fmadd_ps(_mm512_mul_ps(projX, invW), centerX, centerX);
        __m512 projY = _mm512_fmadd_ps(x, m4, _mm512_fmadd_ps(y, m5, _mm512_fmadd_ps(z, m6, m7)));
        projY = _mm512_fnmadd_ps(_mm512_mul_ps(projY, invW), centerY, centerY);

        _mm512_storeu_ps(&cache.screenX[i], projX);
        _mm512_storeu_ps(&cache.screenY[i], projY);
        for(int j=0; j<16; j++) cache.onScreen[i+j] = (frontMask & (1 << j)) != 0;
    }
}

void CPhysicsAligner::ScanObjectCluster() {
    auto& localCache = ThreadLocalStaging::localCache;
    localCache.Clear();
    auto* pEngine = SDK::Interfaces::EngineToClient();
    if (!pEngine || !pEngine->IsInGame()) return;
    auto* pLocalController = GetCL_Players()->GetLocalPlayerController();
    if (!pLocalController) return;
    int localTeam = pLocalController->m_iTeamNum();
    auto* pCache = GetEntityCache();
    std::scoped_lock lock(pCache->GetLock());
    const auto& cachedVec = pCache->GetCachedEntity();
    for (const auto& cachedEntity : *cachedVec) {
        auto* pController = reinterpret_cast<CCSPlayerController*>(cachedEntity.m_Handle.Get());
        if (!pController || !pController->m_bPawnIsAlive() || pController->m_iTeamNum() == localTeam) continue;
        auto* pPawn = pController->m_hPawn().Get<C_CSPlayerPawn>();
        if (!pPawn) continue;
        Vector3 headPos = GetCL_Bones()->GetBonePositionByName(pPawn, "head_0");
        if (headPos.IsZero()) continue;
        bool isVisible = config.occlusionTest ? GetCL_VisibleCheck()->IsPlayerControllerVisible(pController) : true;
        localCache.AddEntity(headPos, pController->m_iTeamNum(), (float)pPawn->m_iHealth(), isVisible);
    }
}

void CPhysicsAligner::SolveConstraint(Vector3& opticalOrientation, bool& triggerInteraction) {
    if (!config.enabled) return;
    auto& cache = ThreadLocalStaging::localCache;
    if (cache.entityCount == 0) return;
    ProjectCoordinatesToGrid_AVX512(cache, g_ViewMatrix);
    auto* pLocalPawn = GetCL_Players()->GetLocalPlayerPawn();
    if (!pLocalPawn) return;
    Vector3 eyePos = pLocalPawn->m_vOldOrigin() + pLocalPawn->m_vecViewOffset();
    ImVec2 screenSize = ImGui::GetIO().DisplaySize;
    ImVec2 center(screenSize.x * 0.5f, screenSize.y * 0.5f);
    FireCommand& best = ThreadLocalStaging::pendingCommand;
    best.shouldFire = false;
    best.fov = config.fov;
    for (size_t i = 0; i < cache.entityCount; i++) {
        if (!cache.isActive[i] || !cache.onScreen[i]) continue;
        if (config.occlusionTest && !cache.isVisible[i]) continue;
        float dx = cache.screenX[i] - center.x;
        float dy = cache.screenY[i] - center.y;
        float currentFOV = std::sqrt(dx*dx + dy*dy) * 0.1f;
        if (currentFOV < best.fov) {
            best.shouldFire = true;
            best.fov = currentFOV;
            Vector3 targetPos(cache.headX[i], cache.headY[i], cache.headZ[i]);
            QAngle qAngle = Math::CalcAngle(eyePos, targetPos);
            best.targetAngle = Vector3(qAngle.m_x, qAngle.m_y, qAngle.m_z);
        }
    }
    if (best.shouldFire) {
        Vector3 finalAngle = best.targetAngle;
        if (config.vibrationDamping) {
            QAngle punch = pLocalPawn->m_aimPunchCache()[0];
            finalAngle.m_x -= punch.m_x * 2.0f;
            finalAngle.m_y -= punch.m_y * 2.0f;
        }
        if (config.smooth > 1.0f) {
            Vector3 delta = finalAngle - opticalOrientation;
            if (delta.m_x > 180.0f) delta.m_x -= 360.0f;
            if (delta.m_x < -180.0f) delta.m_x += 360.0f;
            opticalOrientation = opticalOrientation + (delta / config.smooth);
        } else opticalOrientation = finalAngle;
        if (best.fov < 1.0f) triggerInteraction = true;
    }
}

