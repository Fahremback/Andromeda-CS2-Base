#pragma once
#include "../../Common/Common.hpp"
#include "../../CS2/SDK/Math/Vector3.hpp"
#include "../../CS2/SDK/Math/Matrix.hpp"
#include <cstdint>
#include <atomic>
#include <intrin.h>

class CAimbot
{
public:
    struct alignas(64) AimbotConfig
    {
        bool enabled = true;
        bool triggerBot = false;
        bool autoWall = true;
        float smooth = 1.0f;
        float fov = 10.0f;
        int boneTarget = 6; 
        int minDamage = 1;
        bool visibilityCheck = true; // Restaurado
        bool recoilControl = true;
        float recoilControlY = 2.0f;
        float recoilControlX = 2.0f;
    };

    struct alignas(64) SoAEntityCache
    {
        static constexpr size_t MAX_ENTITIES = 256;
        float headX[MAX_ENTITIES];
        float headY[MAX_ENTITIES];
        float headZ[MAX_ENTITIES];
        uint32_t teamId[MAX_ENTITIES];
        bool isActive[MAX_ENTITIES];
        bool isVisible[MAX_ENTITIES];
        float health[MAX_ENTITIES];
        size_t entityCount = 0;
        void Clear() { entityCount = 0; memset(isActive, 0, sizeof(isActive)); }
        void AddEntity(const Vector3& headPos, uint32_t team, float hp, bool visible) {
            if (entityCount >= MAX_ENTITIES) return;
            headX[entityCount] = headPos.m_x; headY[entityCount] = headPos.m_y; headZ[entityCount] = headPos.m_z;
            teamId[entityCount] = team; health[entityCount] = hp; isVisible[entityCount] = visible; isActive[entityCount] = true;
            entityCount++;
        }
    };

    struct alignas(64) FireCommand {
        bool shouldFire = false;
        Vector3 targetAngle{0,0,0};
        float fov = 999.0f;
    };

    struct ThreadLocalStaging {
        static thread_local SoAEntityCache localCache;
        static thread_local FireCommand pendingCommand;
    };

private:
    static inline AimbotConfig config;
    static __m512 fast_rsqrt14_ps(__m512 v);

public:
    static void Initialize();
    static void Execute(Vector3& viewAngles, bool& shouldShoot);
    static AimbotConfig& GetConfig() { return config; }
    static int GetOptimalThreadCount() { 
        SYSTEM_INFO sysInfo; GetSystemInfo(&sysInfo); return sysInfo.dwNumberOfProcessors; 
    }
    static bool HasAVX512();
};
