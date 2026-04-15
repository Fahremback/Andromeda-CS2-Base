#include "CAimbot.hpp"
#include "../../CS2/SDK/Math/Math.hpp"
#include "../../AndromedaClient/Render/CRender.hpp"
#include <immintrin.h>
#include <cmath>
#include <algorithm>

thread_local CAimbot::SoAEntityCache CAimbot::ThreadLocalStaging::localCache;
thread_local CAimbot::FireCommand CAimbot::ThreadLocalStaging::pendingCommand;

void CAimbot::Initialize() {
    config = AimbotConfig{};
}

__m512 CAimbot::fast_rsqrt14_ps(__m512 v) {
    return _mm512_rsqrt14_ps(v);
}

void CAimbot::Execute(Vector3& viewAngles, bool& shouldShoot) {
    if (!config.enabled) return;

    auto& cache = ThreadLocalStaging::localCache;
    if (cache.entityCount == 0) return;

    FireCommand& best = ThreadLocalStaging::pendingCommand;
    best.shouldFire = false;
    best.fov = config.fov;

    for (size_t i = 0; i < cache.entityCount; i++) {
        if (!cache.isActive[i]) continue;
        if (config.visibilityCheck && !cache.isVisible[i]) continue;

        Vector3 headPos(cache.headX[i], cache.headY[i], cache.headZ[i]);
        ImVec2 screenPos;
        
        if (Math::WorldToScreen(headPos, screenPos)) {
            ImVec2 screenSize = ImGui::GetIO().DisplaySize;
            ImVec2 center(screenSize.x * 0.5f, screenSize.y * 0.5f);
            
            float dx = screenPos.x - center.x;
            float dy = screenPos.y - center.y;
            float currentFOV = std::sqrt(dx*dx + dy*dy) * 0.1f; // FOV aproximado

            if (currentFOV < best.fov) {
                best.shouldFire = true;
                best.fov = currentFOV;
                
                QAngle qAngle = Math::CalcAngle(Vector3(0,0,0), headPos); // Posição local mock
                best.targetAngle = Vector3(qAngle.m_x, qAngle.m_y, qAngle.m_z);
            }
        }
    }

    if (best.shouldFire) {
        if (config.smooth > 1.0f) {
            Vector3 delta = best.targetAngle - viewAngles;
            viewAngles = viewAngles + (delta / config.smooth);
        } else {
            viewAngles = best.targetAngle;
        }
        
        if (best.fov < 1.0f) shouldShoot = true;
    }
}

bool CAimbot::HasAVX512() {
    int cpuInfo[4];
    __cpuidex(cpuInfo, 7, 0);
    return (cpuInfo[1] & (1 << 16)) != 0;
}
