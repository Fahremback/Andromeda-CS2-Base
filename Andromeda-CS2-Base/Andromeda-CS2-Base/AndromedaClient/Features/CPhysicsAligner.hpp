#pragma once
#include "../../Common/Common.hpp"
#include "../../CS2/SDK/Math/Vector3.hpp"
#include "../../CS2/SDK/Math/Matrix.hpp"
#include <cstdint>
#include <atomic>
#include <intrin.h>

class CPhysicsAligner
{
public:
    struct alignas(64) BallSimulationParams
    {
        bool enabled = true;
        bool triggerInteraction = false;
        bool phaseBypass = true;
        float smooth = 1.0f;
        float fov = 10.0f;
        int anchorPoint = 6;
        int minDamage = 1;
        bool occlusionTest = true;
        bool vibrationDamping = true;
        float vibrationDampingY = 2.0f;
        float vibrationDampingX = 2.0f;
    };

    struct alignas(64) FireCommand {
        bool shouldFire = false;
        Vector3 targetAngle{0,0,0};
        float fov = 999.0f;
    };

    struct alignas(64) SoAEntityCache
    {
        static constexpr size_t MAX_ENTITIES = 256;
        alignas(64) float headX[MAX_ENTITIES];
        alignas(64) float headY[MAX_ENTITIES];
        alignas(64) float headZ[MAX_ENTITIES];
        alignas(64) float screenX[MAX_ENTITIES];
        alignas(64) float screenY[MAX_ENTITIES];
        alignas(64) bool isActive[MAX_ENTITIES];
        alignas(64) bool isVisible[MAX_ENTITIES];
        alignas(64) bool onScreen[MAX_ENTITIES];
        alignas(64) uint32_t teamId[MAX_ENTITIES];
        alignas(64) float health[MAX_ENTITIES];
        
        size_t entityCount = 0;
        
        void Clear() { entityCount = 0; memset(isActive, 0, sizeof(isActive)); }
        void AddEntity(const Vector3& headPos, uint32_t team, float hp, bool visible) {
            if (entityCount >= MAX_ENTITIES) return;
            headX[entityCount] = headPos.m_x; headY[entityCount] = headPos.m_y; headZ[entityCount] = headPos.m_z;
            teamId[entityCount] = team; health[entityCount] = hp; isVisible[entityCount] = visible; isActive[entityCount] = true;
            entityCount++;
        }
    };

    class alignas(64) ArenaAllocator {
    private:
        static constexpr size_t SIZE = 8 * 1024 * 1024;
        alignas(64) uint8_t buffer[SIZE];
        std::atomic<size_t> offset{0};
    public:
        void* Alloc(size_t s) {
            size_t curr = offset.fetch_add((s + 63) & ~63, std::memory_order_relaxed);
            return (curr + s <= SIZE) ? &buffer[curr] : nullptr;
        }
        void Reset() { offset.store(0, std::memory_order_relaxed); }
    };

    struct ThreadLocalStaging {
        static thread_local SoAEntityCache localCache;
        static thread_local FireCommand pendingCommand;
        static thread_local ArenaAllocator localArena;
    };

private:
    static inline BallSimulationParams config;
    static void ProjectCoordinatesToGrid_AVX512(SoAEntityCache& cache, const VMatrix& viewMatrix);
    static __m512 fast_rsqrt14_ps(__m512 v);

public:
    static void Initialize();
    static void ScanObjectCluster();
    static void SolveConstraint(Vector3& opticalOrientation, bool& triggerInteraction);
    static BallSimulationParams& GetConfig() { return config; }
    
    static inline int GetOptimalThreadCount() { 
        SYSTEM_INFO sysInfo; GetSystemInfo(&sysInfo); return sysInfo.dwNumberOfProcessors; 
    }
    static inline bool HasAVX512() { 
        int cpuInfo[4]; __cpuidex(cpuInfo, 7, 0); return (cpuInfo[1] & (1 << 16)) != 0; 
    }
};
