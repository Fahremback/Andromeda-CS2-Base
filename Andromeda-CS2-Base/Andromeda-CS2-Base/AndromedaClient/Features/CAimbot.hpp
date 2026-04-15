#pragma once
#include "../Common/Math/Vector3.hpp"
#include "../Common/Math/Matrix4x4.hpp"
#include <cstdint>
#include <atomic>

// ============================================================================
// AIMBOT ULTRA-OTIMIZADO - ANDROMEDA CS2
// Tecnologias: SIMD AVX-512, SoA Cache, Arena Allocator, MPSC Ring-Buffer
// ============================================================================

namespace Andromeda::Features
{
    // Configurações do Aimbot alinhadas para cache
    struct alignas(64) AimbotConfig
    {
        bool enabled = true;
        bool triggerBot = false;
        bool autoWall = true;
        float smooth = 0.0f;
        float fov = 0.0f;
        int boneTarget = 6; // Head
        int minDamage = 1;
        bool visibilityCheck = true;
        bool recoilControl = false;
        float recoilControlY = 0.0f;
        float recoilControlX = 0.0f;
    };

    // Structure of Arrays para Entity Cache - Otimizado para L1 Cache
    struct alignas(64) SoAEntityCache
    {
        static constexpr size_t MAX_ENTITIES = 256;
        
        // Coordenadas contíguas para acesso SIMD
        float posX[MAX_ENTITIES];
        float posY[MAX_ENTITIES];
        float posZ[MAX_ENTITIES];
        
        // Bone positions (head)
        float headX[MAX_ENTITIES];
        float headY[MAX_ENTITIES];
        float headZ[MAX_ENTITIES];
        
        // Dados de estado
        uint32_t teamId[MAX_ENTITIES];
        bool isActive[MAX_ENTITIES];
        bool isVisible[MAX_ENTITIES];
        float health[MAX_ENTITIES];
        float distance[MAX_ENTITIES];
        
        size_t entityCount = 0;
        
        void Clear()
        {
            entityCount = 0;
            _mm512_store_ps(posX, _mm512_setzero_ps());
            _mm512_store_ps(posY, _mm512_setzero_ps());
            _mm512_store_ps(posZ, _mm512_setzero_ps());
        }
        
        void AddEntity(uint32_t index, const Vector3& pos, const Vector3& headPos, 
                       uint32_t team, float hp, float dist)
        {
            if (entityCount >= MAX_ENTITIES) return;
            
            posX[entityCount] = pos.x;
            posY[entityCount] = pos.y;
            posZ[entityCount] = pos.z;
            
            headX[entityCount] = headPos.x;
            headY[entityCount] = headPos.y;
            headZ[entityCount] = headPos.z;
            
            teamId[entityCount] = team;
            isActive[entityCount] = true;
            isVisible[entityCount] = false;
            health[entityCount] = hp;
            distance[entityCount] = dist;
            
            entityCount++;
        }
    };

    // Comando de disparo thread-safe
    struct alignas(64) FireCommand
    {
        std::atomic<bool> shouldFire{false};
        Vector3 targetAngle{0, 0, 0};
        Vector3 targetPosition{0, 0, 0};
        uint32_t targetIndex = 0;
        float damage = 0;
        int64_t timestamp = 0;
        char padding[64 - sizeof(std::atomic<bool>) - sizeof(Vector3)*2 - sizeof(uint32_t) - sizeof(float) - sizeof(int64_t)];
    };

    // MPSC Ring Buffer para comunicação lock-free entre threads
    class alignas(64) MPSCRingBuffer
    {
    private:
        static constexpr size_t BUFFER_SIZE = 1024;
        
        struct Entry
        {
            FireCommand command;
            std::atomic<uint64_t> sequence{0};
        };
        
        alignas(64) Entry buffer[BUFFER_SIZE];
        std::atomic<uint64_t> head{0};
        std::atomic<uint64_t> tail{0};
        
    public:
        bool TryPush(const FireCommand& cmd)
        {
            const uint64_t currentTail = tail.load(std::memory_order_relaxed);
            const uint64_t nextTail = (currentTail + 1) % BUFFER_SIZE;
            
            if (nextTail == head.load(std::memory_order_acquire))
                return false; // Buffer cheio
            
            buffer[currentTail].command = cmd;
            buffer[currentTail].sequence.store(currentTail + 1, std::memory_order_release);
            tail.store(nextTail, std::memory_order_release);
            
            return true;
        }
        
        bool TryPop(FireCommand& outCmd)
        {
            const uint64_t currentHead = head.load(std::memory_order_relaxed);
            
            if (currentHead == tail.load(std::memory_order_acquire))
                return false; // Buffer vazio
            
            const uint64_t sequence = buffer[currentHead].sequence.load(std::memory_order_acquire);
            if (sequence != currentHead + 1)
                return false; // Race condition detectada
            
            outCmd = buffer[currentHead].command;
            head.store((currentHead + 1) % BUFFER_SIZE, std::memory_order_release);
            
            return true;
        }
        
        bool IsEmpty() const
        {
            return head.load(std::memory_order_acquire) == tail.load(std::memory_order_acquire);
        }
    };

    // Arena Allocator para cálculos temporários (8MB aligned)
    class alignas(64) ArenaAllocator
    {
    private:
        static constexpr size_t ARENA_SIZE = 8 * 1024 * 1024; // 8MB
        alignas(64) uint8_t arena[ARENA_SIZE];
        std::atomic<size_t> offset{0};
        
    public:
        void* Allocate(size_t size, size_t alignment = 64)
        {
            size_t currentOffset = offset.load(std::memory_order_relaxed);
            
            // Alinhamento
            size_t alignedOffset = (currentOffset + alignment - 1) & ~(alignment - 1);
            size_t newOffset = alignedOffset + size;
            
            if (newOffset > ARENA_SIZE)
                return nullptr; // Arena cheia
            
            if (!offset.compare_exchange_weak(currentOffset, newOffset, std::memory_order_acq_rel))
                return Allocate(size, alignment); // Retry
            
            return &arena[alignedOffset];
        }
        
        void Reset()
        {
            offset.store(0, std::memory_order_release);
        }
    };

    // Thread-Local Staging para evitar race conditions
    struct alignas(64) ThreadLocalStaging
    {
        static thread_local SoAEntityCache localCache;
        static thread_local ArenaAllocator localArena;
        static thread_local FireCommand pendingCommand;
        static thread_local bool isProcessing;
    };

    // Classe principal do Aimbot
    class CAimbot
    {
    private:
        static inline AimbotConfig config;
        static inline MPSCRingBuffer fireQueue;
        static inline ArenaAllocator globalArena;
        
        // Métodos otimizados com SIMD
        static __m512 NormalizeVectorFast_AVX512(__m512 vec);
        static __m512 CalcAngle_FMA(__m512 srcPos, __m512 dstPos);
        
        // Processamento batch de alvos com AVX-512
        static void ProcessTargets_SIMD(SoAEntityCache& cache, FireCommand& bestTarget);
        
        // AutoWall com cálculo de dano mínimo
        static float AutoWall_CalculateDamage(const Vector3& startPos, const Vector3& endPos, 
                                               int minDamage, bool& canPenetrate);
        
        // Trigger Bot instantâneo
        static bool TriggerBot_Execute(const Vector3& viewAngle, const SoAEntityCache& cache);
        
        // Smooth com rsqrt14
        static Vector3 ApplySmooth(const Vector3& current, const Vector3& target, float smooth);
        
    public:
        static void Initialize();
        static void Execute(Vector3& viewAngles, bool& shouldShoot);
        static void UpdateConfig(const AimbotConfig& newConfig);
        static const AimbotConfig& GetConfig() { return config; }
        
        // Debug via ring buffer
        static bool GetDebugInfo(FireCommand& cmd);
        
        // System info
        static bool HasAVX512() 
        { 
            int cpuInfo[4];
            __cpuidex(cpuInfo, 7, 0);
            return (cpuInfo[1] & (1 << 16)) != 0; // AVX-512F bit
        }
        static int GetOptimalThreadCount()
        {
            SYSTEM_INFO sysInfo;
            GetSystemInfo(&sysInfo);
            return sysInfo.dwNumberOfProcessors;
        }
    };
}
