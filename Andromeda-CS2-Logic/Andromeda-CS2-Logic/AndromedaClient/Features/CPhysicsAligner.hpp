#pragma once
#include "../../Common/Common.hpp"
#include "../../CS2/SDK/Math/Vector3.hpp"
#include "../../CS2/SDK/Math/Matrix.hpp"
#include "CPhysicsCriticalPhases.hpp"
#include <cstdint>
#include <atomic>
#include <array>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <intrin.h>

class CPhysicsAligner
{
public:
    static inline CPhysicsCriticalPhases::SensorMatrixState ioState;

    struct alignas(64) BallSimulationParams
    {
        bool enabled = true;
        bool triggerInteraction = false;
        bool phaseBypass = true;
        float smooth = 1.0f;
        float fov = 10.0f;
        int anchorPoint = 6;
        int minDamage = 1;
        bool backtrackEnabled = false;
        bool occlusionTest = true;
        bool vibrationDamping = true;
        float vibrationDampingY = 2.0f;
        float vibrationDampingX = 2.0f;
        uint32_t processIntervalMs = 8;
        uint32_t visibilityCacheMs = 50;
    };

    struct alignas(64) FireCommand {
        bool shouldFire = false;
        Vector3 targetAngle{0,0,0};
        float fov = 999.0f;
    };

    // Resultado pré-computado consumido pelo CreateMove (double-buffer)
    struct alignas(64) AimbotResult {
        bool ready = false;
        bool shouldFire = false;
        Vector3 finalAngle{0, 0, 0};
        bool triggerInteraction = false;
    };

    struct PerfSnapshot {
        uint32_t lastProcessUs = 0;
        uint32_t averageProcessUs = 0;
        uint32_t maxProcessUs = 0;
        uint32_t lastEntityCount = 0;
        uint32_t submittedRequests = 0;
        uint32_t throttledRequests = 0;
        uint32_t busyRequests = 0;
        uint32_t effectiveIntervalMs = 0;
        bool hotTracking = false;
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
        
        void Clear() {
            entityCount = 0;
            memset(isActive, 0, sizeof(isActive));
            memset(isVisible, 0, sizeof(isVisible));
            memset(onScreen, 0, sizeof(onScreen));
        }
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
        uint8_t* buffer = nullptr;
        std::atomic<size_t> offset{0};
    public:
        ArenaAllocator();
        ~ArenaAllocator();
        ArenaAllocator(const ArenaAllocator&) = delete;
        ArenaAllocator& operator=(const ArenaAllocator&) = delete;

        void* Alloc(size_t s) {
            size_t curr = offset.fetch_add((s + 63) & ~63, std::memory_order_relaxed);
            return (curr + s <= SIZE) ? &buffer[curr] : nullptr;
        }
        void Reset() { offset.store(0, std::memory_order_relaxed); }
    };

    struct alignas(64) RaycastResult {
        enum class Kind : uint8_t { Raycast = 0, Snapshot = 1 };
        Kind kind = Kind::Raycast;
        uint16_t entityIndex = 0;
        bool reachable = false;
        uint16_t snapshotIndex = 0;
        float score = 0.0f;
        Vector3 snapshotPosition{0, 0, 0};
    };

    struct alignas(64) SnapshotState
    {
        static constexpr size_t MAX_HISTORY = 32;
        alignas(64) float headX[SoAEntityCache::MAX_ENTITIES];
        alignas(64) float headY[SoAEntityCache::MAX_ENTITIES];
        alignas(64) float headZ[SoAEntityCache::MAX_ENTITIES];
        alignas(64) float screenX[SoAEntityCache::MAX_ENTITIES];
        alignas(64) float screenY[SoAEntityCache::MAX_ENTITIES];
        alignas(64) bool isActive[SoAEntityCache::MAX_ENTITIES];
        alignas(64) bool isVisible[SoAEntityCache::MAX_ENTITIES];
        alignas(64) bool onScreen[SoAEntityCache::MAX_ENTITIES];
        uint64_t captureTimeUs = 0;
        size_t entityCount = 0;
    };

    class alignas(64) MPSCRingBuffer {
    private:
        static constexpr uint32_t CAPACITY = 512;
        static_assert((CAPACITY & (CAPACITY - 1U)) == 0U, "CAPACITY must be power of two");
        alignas(64) std::array<RaycastResult, CAPACITY> buffer{};
        alignas(64) std::atomic<uint32_t> head{0};
        alignas(64) std::atomic<uint32_t> tail{0};

    public:
        bool TryPush(const RaycastResult& item);
        bool TryPop(RaycastResult& out);
        void Reset();
    };

    struct ThreadLocalStaging {
        static thread_local SoAEntityCache localCache;
        static thread_local FireCommand pendingCommand;
        static thread_local ArenaAllocator localArena;
        static thread_local SnapshotState snapshotHistory[SnapshotState::MAX_HISTORY];
        static thread_local size_t snapshotWriteIndex;
        static thread_local size_t snapshotCount;
    };

private:
    enum class JobType : uint8_t { None = 0, Reachability = 1, SnapshotRollback = 2 };
    struct alignas(64) JobContext {
        JobType type = JobType::None;
        SoAEntityCache* cache = nullptr;
        Vector3 eyePos{0, 0, 0};
        SnapshotState* snapshots = nullptr;
        size_t snapshotCount = 0;
        float centerX = 0.0f;
        float centerY = 0.0f;
        float laneLimit = 0.0f;
    };

    static inline BallSimulationParams config;
    static inline MPSCRingBuffer raycastResults;
    static inline std::vector<std::thread> workerThreads;
    static inline std::atomic<bool> workerStop{ false };
    static inline std::atomic<uint32_t> workerGeneration{ 0 };
    static inline std::atomic<uint32_t> workerCompleted{ 0 };
    static inline std::atomic<uint32_t> workerCount{ 0 };
    static inline std::mutex workerMutex;
    static inline std::condition_variable workerCv;
    static inline JobContext activeJob;

    // Double-buffered view matrix snapshot for async thread safety
    static inline VMatrix g_ViewMatrixSnapshot;
    static inline std::atomic<bool> g_ViewMatrixSnapshotReady{false};

    // Async processing thread (separate from worker job threads)
    static inline std::thread asyncProcessThread;
    static inline std::atomic<bool> asyncThreadStop{ false };
    static inline std::atomic<bool> asyncProcessRequested{ false };
    static inline std::atomic<bool> asyncProcessInFlight{ false };
    static inline std::atomic<bool> asyncProcessComplete{ false };
    static inline std::atomic<uint64_t> lastAsyncRequestUs{ 0 };
    static inline std::atomic<uint64_t> lastTargetTrackUs{ 0 };
    static inline std::mutex asyncRequestMutex;
    static inline std::condition_variable asyncRequestCv;
    static inline AimbotResult latestResult{};
    static inline std::mutex asyncResultMutex;
    static inline std::atomic<uint32_t> perfLastProcessUs{ 0 };
    static inline std::atomic<uint32_t> perfAverageProcessUs{ 0 };
    static inline std::atomic<uint32_t> perfMaxProcessUs{ 0 };
    static inline std::atomic<uint32_t> perfLastEntityCount{ 0 };
    static inline std::atomic<uint32_t> perfSubmittedRequests{ 0 };
    static inline std::atomic<uint32_t> perfThrottledRequests{ 0 };
    static inline std::atomic<uint32_t> perfBusyRequests{ 0 };
    static inline std::atomic<uint32_t> perfEffectiveIntervalMs{ 0 };
    static inline std::atomic<bool> perfHotTracking{ false };
    static void AsyncProcessThreadMain();
    static void StartAsyncThread();

    static void ProjectCoordinatesToGrid_AVX512(SoAEntityCache& cache, const VMatrix& viewMatrix, const Vector3& sensorPos);
    static __m512 fast_rsqrt14_ps(__m512 v);
    static Vector3 NormalizeVectorFast(const Vector3& v);
    static void InitializeJobSystem();
    static void ShutdownJobSystem();
    static void DispatchParallelJob(JobType type);
    static void EnsureWorkersSpawned();
    static void JobWorkerMain(uint32_t workerIndex);
    static void ProcessReachabilityWorker(uint32_t workerIndex, uint32_t totalWorkers, const JobContext& context);
    static void ProcessSnapshotWorker(uint32_t workerIndex, uint32_t totalWorkers, const JobContext& context);
    static void ResolvePhaseBypassReachability(SoAEntityCache& cache, const Vector3& eyePos);
    static uint64_t GetTimestampUs();
    static void CaptureDeterministicSnapshot(const SoAEntityCache& cache, uint64_t nowUs);
    static void InvalidateStaleSnapshots(uint64_t nowUs, uint64_t staleWindowUs);
    static bool ResolveTemporalRollbackTarget(const Vector3& eyePos, float& inOutBestDistance, Vector3& outTargetPos);

public:
    static void Initialize();
    static void OnDestroy();
    static void ScanObjectCluster();
    static void SolveConstraint(Vector3& opticalOrientation, bool& triggerInteraction);
    static BallSimulationParams& GetConfig() { return config; }

    // Sistema async: request processamento no frame, consumir resultado no CreateMove
    static void RequestAsyncProcess();
    static bool ConsumeResult(AimbotResult& out);
    static PerfSnapshot GetPerfSnapshot();
    
    static inline int GetOptimalThreadCount() { 
        SYSTEM_INFO sysInfo; GetSystemInfo(&sysInfo); return sysInfo.dwNumberOfProcessors; 
    }
    static inline bool HasAVX512() { 
        int cpuInfo[4]; __cpuidex(cpuInfo, 7, 0); return (cpuInfo[1] & (1 << 16)) != 0; 
    }
};
