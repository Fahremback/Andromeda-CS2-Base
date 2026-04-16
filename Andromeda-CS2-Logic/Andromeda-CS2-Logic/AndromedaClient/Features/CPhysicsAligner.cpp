
#include "CPhysicsAligner.hpp"
#include "../../CS2/SDK/Math/Math.hpp"
#include "../../AndromedaClient/Render/CRender.hpp"
#include "../../GameClient/CEntityCache/CEntityCache.hpp"
#include "../../GameClient/CL_Players.hpp"
#include "../../GameClient/CL_VisibleCheck.hpp"
#include "../../GameClient/CL_Bones.hpp"
#include "../../Common/CTelemetry.hpp"
#include <CS2/SDK/Interface/IEngineToClient.hpp>
#include <CS2/Hook/Hook_GetMatricesForView.hpp>
#include <immintrin.h>
#include <cmath>
#include <algorithm>
#include <mutex>
#include <thread>
#include <vector>
#include <limits>
#include <memory>
#include <array>
#include <type_traits>
#include <malloc.h>

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
#include <chrono>
#include <cstdlib>

thread_local CPhysicsAligner::SoAEntityCache CPhysicsAligner::ThreadLocalStaging::localCache;
thread_local CPhysicsAligner::FireCommand CPhysicsAligner::ThreadLocalStaging::pendingCommand;
thread_local CPhysicsAligner::ArenaAllocator CPhysicsAligner::ThreadLocalStaging::localArena;
thread_local CPhysicsAligner::SnapshotState CPhysicsAligner::ThreadLocalStaging::snapshotHistory[CPhysicsAligner::SnapshotState::MAX_HISTORY];
thread_local size_t CPhysicsAligner::ThreadLocalStaging::snapshotWriteIndex = 0;
thread_local size_t CPhysicsAligner::ThreadLocalStaging::snapshotCount = 0;

namespace
{
    constexpr uint32_t kMaxWorkers = 16;
    constexpr uint64_t kUsPerMs = 1000ULL;

    struct VisibilityCacheEntry
    {
        C_CSPlayerPawn* pawn = nullptr;
        uint64_t sampledAtUs = 0;
        bool visible = false;
    };

    std::array<VisibilityCacheEntry, CPhysicsAligner::SoAEntityCache::MAX_ENTITIES> g_visibilityCache{};
    std::mutex g_visibilityCacheMutex;

    bool SampleVisibilityCached(C_CSPlayerPawn* pPawn, uint64_t nowUs, uint32_t cacheMs)
    {
        if (!pPawn)
            return false;

        const uint64_t ttlUs = static_cast<uint64_t>(cacheMs) * kUsPerMs;
        std::lock_guard<std::mutex> lock(g_visibilityCacheMutex);

        for (auto& entry : g_visibilityCache)
        {
            if (entry.pawn != pPawn)
                continue;

            if (entry.sampledAtUs != 0 && ttlUs != 0 && (nowUs - entry.sampledAtUs) < ttlUs)
                return entry.visible;

            entry.visible = GetCL_VisibleCheck()->IsPlayerPawnVisible(pPawn);
            entry.sampledAtUs = nowUs;
            return entry.visible;
        }

        for (auto& entry : g_visibilityCache)
        {
            if (entry.pawn != nullptr)
                continue;

            entry.pawn = pPawn;
            entry.visible = GetCL_VisibleCheck()->IsPlayerPawnVisible(pPawn);
            entry.sampledAtUs = nowUs;
            return entry.visible;
        }

        auto& slot = g_visibilityCache[nowUs % g_visibilityCache.size()];
        slot.pawn = pPawn;
        slot.visible = GetCL_VisibleCheck()->IsPlayerPawnVisible(pPawn);
        slot.sampledAtUs = nowUs;
        return slot.visible;
    }
}

CPhysicsAligner::ArenaAllocator::ArenaAllocator() {
    buffer = reinterpret_cast<uint8_t*>(_aligned_malloc(SIZE, SIZE));
    if (!buffer)
        buffer = reinterpret_cast<uint8_t*>(_aligned_malloc(SIZE, 64));
    offset.store(0, std::memory_order_relaxed);
}

CPhysicsAligner::ArenaAllocator::~ArenaAllocator() {
    if (buffer) {
        _aligned_free(buffer);
        buffer = nullptr;
    }
}

bool CPhysicsAligner::MPSCRingBuffer::TryPush(const RaycastResult& item) {
    const uint32_t pos = head.fetch_add(1, std::memory_order_acq_rel);
    const uint32_t localTail = tail.load(std::memory_order_acquire);
    if ((pos - localTail) >= CAPACITY)
        return false;
    buffer[pos & (CAPACITY - 1U)] = item;
    return true;
}

bool CPhysicsAligner::MPSCRingBuffer::TryPop(RaycastResult& out) {
    const uint32_t localHead = head.load(std::memory_order_acquire);
    const uint32_t localTail = tail.load(std::memory_order_relaxed);
    if (localTail >= localHead)
        return false;

    out = buffer[localTail & (CAPACITY - 1U)];
    tail.store(localTail + 1, std::memory_order_release);
    return true;
}

void CPhysicsAligner::MPSCRingBuffer::Reset() {
    head.store(0, std::memory_order_release);
    tail.store(0, std::memory_order_release);
}

void CPhysicsAligner::OnDestroy()
{
    // Stop async thread
    {
        std::lock_guard<std::mutex> lock(asyncRequestMutex);
        asyncThreadStop = true;
    }
    asyncRequestCv.notify_all();
    if (asyncProcessThread.joinable())
        asyncProcessThread.join();

    // Stop worker threads
    ShutdownJobSystem();
}

void CPhysicsAligner::Initialize()
{

    config = BallSimulationParams{};
    InitializeJobSystem();
    StartAsyncThread();
}

void CPhysicsAligner::StartAsyncThread() {
    if (asyncProcessThread.joinable())
        return;
    asyncThreadStop.store(false, std::memory_order_release);
    asyncProcessRequested.store(false, std::memory_order_release);
    asyncProcessComplete.store(false, std::memory_order_release);
    asyncProcessThread = std::thread(&CPhysicsAligner::AsyncProcessThreadMain);
}

void CPhysicsAligner::InitializeJobSystem() {
    workerStop.store(false, std::memory_order_release);
    workerGeneration.store(0, std::memory_order_release);
    workerCompleted.store(0, std::memory_order_release);
    workerCount.store(0, std::memory_order_release);
}

void CPhysicsAligner::ShutdownJobSystem() {
    if (workerThreads.empty() && !asyncProcessThread.joinable())
        return;

    asyncThreadStop.store(true, std::memory_order_release);
    asyncRequestCv.notify_all();
    if (asyncProcessThread.joinable())
        asyncProcessThread.join();

    workerStop.store(true, std::memory_order_release);
    workerCv.notify_all();
    for (auto& thread : workerThreads) {
        if (thread.joinable())
            thread.join();
    }
    workerThreads.clear();
}

void CPhysicsAligner::AsyncProcessThreadMain() {
    while (!asyncThreadStop.load(std::memory_order_acquire)) {
        // Esperar por uma requisição de processamento
        {
            std::unique_lock<std::mutex> requestLock(asyncRequestMutex);
            asyncRequestCv.wait(requestLock, [] {
                return asyncThreadStop.load(std::memory_order_acquire) ||
                    asyncProcessRequested.load(std::memory_order_acquire);
            });
        }

        if (asyncThreadStop.load(std::memory_order_acquire))
            return;

        asyncProcessRequested.store(false, std::memory_order_release);
        const uint64_t processStartUs = GetTimestampUs();

        // Executar o pipeline completo (ScanObjectCluster + SolveConstraint)
        auto& localCache = ThreadLocalStaging::localCache;
        auto& localArena = ThreadLocalStaging::localArena;
        localArena.Reset();
        localCache.Clear();

        auto* pEngine = SDK::Interfaces::EngineToClient();
        if (!pEngine || !pEngine->IsInGame()) {
            asyncProcessInFlight.store(false, std::memory_order_release);
            asyncProcessComplete.store(false, std::memory_order_release);
            continue;
        }

        auto* pLocalController = GetCL_Players()->GetLocalPlayerController();
        if (!pLocalController) {
            asyncProcessInFlight.store(false, std::memory_order_release);
            asyncProcessComplete.store(false, std::memory_order_release);
            continue;
        }

        // === ScanObjectCluster ===
        const uint64_t nowUs = GetTimestampUs();
        int localTeam = pLocalController->m_iTeamNum();
        auto* pCache = GetEntityCache();
        using CachedEntityVec = std::remove_reference_t<decltype(*pCache->GetCachedEntity())>;
        CachedEntityVec snapshot;
        {
            std::scoped_lock lock(pCache->GetLock());
            const auto& cachedVec = pCache->GetCachedEntity();
            snapshot = *cachedVec;
        }

        for (const auto& cachedEntity : snapshot) {
            auto* pController = reinterpret_cast<CCSPlayerController*>(cachedEntity.m_Handle.Get());
            if (!pController || !pController->m_bPawnIsAlive() || pController->m_iTeamNum() == localTeam) continue;
            auto* pPawn = pController->m_hPawn().Get<C_CSPlayerPawn>();
            if (!pPawn) continue;
            Vector3 headPos = GetCL_Bones()->GetBonePositionByName(pPawn, "head_0");
            if (headPos.IsZero()) continue;
            bool isVisible = config.occlusionTest ? SampleVisibilityCached(pPawn, nowUs, config.visibilityCacheMs) : true;
            localCache.AddEntity(headPos, pController->m_iTeamNum(), (float)pPawn->m_iHealth(), isVisible);
        }

        // === SolveConstraint ===
        AimbotResult result{};
        result.ready = true;
        result.shouldFire = false;
        result.triggerInteraction = false;
        result.finalAngle = Vector3(0, 0, 0);

        if (localCache.entityCount > 0) {
            auto* pLocalPawn = GetCL_Players()->GetLocalPlayerPawn();
            if (pLocalPawn) {
                const Vector3 eyePos = pLocalPawn->m_vOldOrigin() + pLocalPawn->m_vecViewOffset();

                // ProjectCoordinates
                {
                    ScopedProfiler _profiler("ProjectCoordinates");
                    ProjectCoordinatesToGrid_AVX512(localCache, g_ViewMatrixSnapshot, eyePos);
                }

                // Reachability
                {
                    ScopedProfiler _profiler("ResolveReachability");
                    ResolvePhaseBypassReachability(localCache, eyePos);
                }

                if (config.backtrackEnabled) {
                    InvalidateStaleSnapshots(nowUs, 200000ULL);
                    CaptureDeterministicSnapshot(localCache, nowUs);
                }

                // Target selection (simplified - same logic as SolveConstraint but on async thread)
                float bestDistance = std::numeric_limits<float>::max();
                size_t bestIndex = 0;

                // Get screen size from ioState (populated by CreateMove, safe for async thread)
                const float centerX_f = ioState.screenWidth * 0.5f;
                const float centerY_f = ioState.screenHeight * 0.5f;
                const float fovLimit = config.fov * 10.0f;

                const size_t stride = 16;
                for (size_t i = 0; i < localCache.entityCount; i += stride) {
                    const size_t remaining = localCache.entityCount - i;
                    const __mmask16 laneMask = static_cast<__mmask16>((remaining >= stride) ? 0xFFFFu : ((1u << remaining) - 1u));

                    const __m512 sx = _mm512_loadu_ps(&localCache.screenX[i]);
                    const __m512 sy = _mm512_loadu_ps(&localCache.screenY[i]);
                    const __m512 centerXS = _mm512_set1_ps(centerX_f);
                    const __m512 centerYS = _mm512_set1_ps(centerY_f);
                    const __m512 dx = _mm512_sub_ps(sx, centerXS);
                    const __m512 dy = _mm512_sub_ps(sy, centerYS);
                    const __m512 distSq = _mm512_fmadd_ps(dx, dx, _mm512_mul_ps(dy, dy));
                    const __m512 invDist = fast_rsqrt14_ps(_mm512_max_ps(distSq, _mm512_set1_ps(1.0e-12f)));
                    const __m512 dist = _mm512_mul_ps(distSq, invDist);

                    __mmask16 validMask = laneMask;
                    for (size_t lane = 0; lane < remaining && lane < stride; ++lane) {
                        const size_t idx = i + lane;
                        const bool visible = !config.occlusionTest || localCache.isVisible[idx];
                        if (!(localCache.isActive[idx] && localCache.onScreen[idx] && visible))
                            validMask &= static_cast<__mmask16>(~(1u << lane));
                    }
                    validMask &= _mm512_mask_cmp_ps_mask(validMask, dist, _mm512_set1_ps(fovLimit), _CMP_LT_OS);

                    while (validMask) {
                        uint32_t lane = 0;
                        while (lane < stride && ((validMask & (1u << lane)) == 0))
                            ++lane;
                        if (lane >= stride)
                            break;
                        validMask &= static_cast<__mmask16>(~(1u << lane));

                        const size_t idx = i + lane;
                        const float preciseDx = localCache.screenX[idx] - centerX_f;
                        const float preciseDy = localCache.screenY[idx] - centerY_f;
                        const float preciseDistance = std::sqrt((preciseDx * preciseDx) + (preciseDy * preciseDy));

                        if (preciseDistance < bestDistance) {
                            bestDistance = preciseDistance;
                            bestIndex = idx;
                            result.shouldFire = true;
                        }
                    }
                }

                // Temporal rollback
                Vector3 temporalTargetPos{0, 0, 0};
                if (config.backtrackEnabled && ResolveTemporalRollbackTarget(eyePos, bestDistance, temporalTargetPos)) {
                    result.shouldFire = true;
                    bestIndex = SoAEntityCache::MAX_ENTITIES;
                }

                if (result.shouldFire) {
                    const Vector3 targetPos = (bestIndex == SoAEntityCache::MAX_ENTITIES)
                        ? temporalTargetPos
                        : Vector3(localCache.headX[bestIndex], localCache.headY[bestIndex], localCache.headZ[bestIndex]);
                    QAngle qAngle = Math::CalcAngle(eyePos, targetPos);
                    Vector3 finalAngle = Vector3(qAngle.m_x, qAngle.m_y, qAngle.m_z);

                    if (config.vibrationDamping) {
                        const QAngle punch = pLocalPawn->m_aimPunchCache()[0];
                        finalAngle.m_x -= punch.m_x * config.vibrationDampingX;
                        finalAngle.m_y -= punch.m_y * config.vibrationDampingY;
                    }

                    if (config.smooth > 1.0f) {
                        // Get current angles from ioState (updated by CreateMove)
                        Vector3 delta = finalAngle - ioState.collisionAngles;
                        while (delta.m_x > 180.0f) delta.m_x -= 360.0f;
                        while (delta.m_x < -180.0f) delta.m_x += 360.0f;
                        while (delta.m_y > 180.0f) delta.m_y -= 360.0f;
                        while (delta.m_y < -180.0f) delta.m_y += 360.0f;
                        finalAngle = ioState.collisionAngles + (delta / config.smooth);
                    }

                    result.finalAngle = finalAngle;
                    if (bestDistance < 1.0f)
                        result.triggerInteraction = true;

                    lastTargetTrackUs.store(nowUs, std::memory_order_release);
                }
            }
        }

        // Store result
        {
            std::lock_guard<std::mutex> lock(asyncResultMutex);
            latestResult = result;
        }

        const uint32_t processDurationUs = static_cast<uint32_t>((std::min<uint64_t>)(GetTimestampUs() - processStartUs, std::numeric_limits<uint32_t>::max()));
        perfLastProcessUs.store(processDurationUs, std::memory_order_release);
        perfLastEntityCount.store(static_cast<uint32_t>(localCache.entityCount), std::memory_order_release);

        const uint32_t previousAverage = perfAverageProcessUs.load(std::memory_order_acquire);
        const uint32_t smoothedAverage = previousAverage == 0
            ? processDurationUs
            : static_cast<uint32_t>((previousAverage * 7ULL + processDurationUs) / 8ULL);
        perfAverageProcessUs.store(smoothedAverage, std::memory_order_release);

        uint32_t previousMax = perfMaxProcessUs.load(std::memory_order_acquire);
        while (processDurationUs > previousMax &&
            !perfMaxProcessUs.compare_exchange_weak(previousMax, processDurationUs, std::memory_order_acq_rel)) {
        }

        asyncProcessInFlight.store(false, std::memory_order_release);
        asyncProcessComplete.store(true, std::memory_order_release);
    }
}

void CPhysicsAligner::RequestAsyncProcess() {
    if (!config.enabled || ioState.screenWidth <= 0.0f || ioState.screenHeight <= 0.0f)
        return;

    const uint64_t nowUs = GetTimestampUs();
    const uint64_t lastTrackedTargetUs = lastTargetTrackUs.load(std::memory_order_acquire);
    const bool hotTracking = lastTrackedTargetUs != 0 && (nowUs - lastTrackedTargetUs) <= 150000ULL;
    const uint32_t effectiveIntervalMs = hotTracking
        ? config.processIntervalMs
        : (std::max)(config.processIntervalMs * 2u, 14u);
    const uint64_t minIntervalUs = static_cast<uint64_t>(effectiveIntervalMs) * kUsPerMs;
    const uint64_t previousRequestUs = lastAsyncRequestUs.load(std::memory_order_acquire);
    perfEffectiveIntervalMs.store(effectiveIntervalMs, std::memory_order_release);
    perfHotTracking.store(hotTracking, std::memory_order_release);

    if (previousRequestUs != 0 && (nowUs - previousRequestUs) < minIntervalUs) {
        perfThrottledRequests.fetch_add(1, std::memory_order_acq_rel);
        return;
    }

    if (asyncProcessInFlight.exchange(true, std::memory_order_acq_rel)) {
        perfBusyRequests.fetch_add(1, std::memory_order_acq_rel);
        return;
    }

    lastAsyncRequestUs.store(nowUs, std::memory_order_release);
    perfSubmittedRequests.fetch_add(1, std::memory_order_acq_rel);
    asyncProcessRequested.store(true, std::memory_order_release);
    asyncProcessComplete.store(false, std::memory_order_release);

    // Snapshot view matrix for async thread (captured on game thread)
    g_ViewMatrixSnapshot = g_ViewMatrix;
    g_ViewMatrixSnapshotReady.store(true, std::memory_order_release);

    asyncRequestCv.notify_one();
}

bool CPhysicsAligner::ConsumeResult(AimbotResult& out) {
    if (!asyncProcessComplete.load(std::memory_order_acquire))
        return false;
    std::lock_guard<std::mutex> lock(asyncResultMutex);
    if (!latestResult.ready)
        return false;
    out = latestResult;
    return true;
}

CPhysicsAligner::PerfSnapshot CPhysicsAligner::GetPerfSnapshot() {
    PerfSnapshot snapshot{};
    snapshot.lastProcessUs = perfLastProcessUs.load(std::memory_order_acquire);
    snapshot.averageProcessUs = perfAverageProcessUs.load(std::memory_order_acquire);
    snapshot.maxProcessUs = perfMaxProcessUs.load(std::memory_order_acquire);
    snapshot.lastEntityCount = perfLastEntityCount.load(std::memory_order_acquire);
    snapshot.submittedRequests = perfSubmittedRequests.load(std::memory_order_acquire);
    snapshot.throttledRequests = perfThrottledRequests.load(std::memory_order_acquire);
    snapshot.busyRequests = perfBusyRequests.load(std::memory_order_acquire);
    snapshot.effectiveIntervalMs = perfEffectiveIntervalMs.load(std::memory_order_acquire);
    snapshot.hotTracking = perfHotTracking.load(std::memory_order_acquire);
    return snapshot;
}

void CPhysicsAligner::EnsureWorkersSpawned() {
    if (!workerThreads.empty())
        return;

    // FIX: Limit workers to avoid competing with game for CPU resources.
    // Use half the available cores, capped at 4 to keep the game responsive.
    const uint32_t desiredWorkers = (std::min<uint32_t>)((std::max)(1, GetOptimalThreadCount() / 2), 4);
    workerCount.store(desiredWorkers, std::memory_order_release);
    workerThreads.reserve(desiredWorkers);
    for (uint32_t i = 0; i < desiredWorkers; ++i)
        workerThreads.emplace_back(&CPhysicsAligner::JobWorkerMain, i);
}

void CPhysicsAligner::DispatchParallelJob(JobType type) {
    EnsureWorkersSpawned();

    const uint32_t totalWorkers = workerCount.load(std::memory_order_acquire);
    if (totalWorkers == 0 || workerThreads.empty()) {
        if (type == JobType::Reachability)
            ProcessReachabilityWorker(0, 1, activeJob);
        else if (type == JobType::SnapshotRollback)
            ProcessSnapshotWorker(0, 1, activeJob);
        return;
    }

    if (totalWorkers == 1) {
        if (type == JobType::Reachability)
            ProcessReachabilityWorker(0, 1, activeJob);
        else if (type == JobType::SnapshotRollback)
            ProcessSnapshotWorker(0, 1, activeJob);
        return;
    }

    activeJob.type = type;
    workerCompleted.store(0, std::memory_order_release);
    {
        std::lock_guard<std::mutex> lock(workerMutex);
        workerGeneration.fetch_add(1, std::memory_order_acq_rel);
    }
    const uint32_t targetCompletions = totalWorkers;
    workerCv.notify_all();

    std::unique_lock<std::mutex> waitLock(workerMutex);
    workerCv.wait(waitLock, [targetCompletions] {
        return workerCompleted.load(std::memory_order_acquire) >= targetCompletions;
    });
}

void CPhysicsAligner::JobWorkerMain(uint32_t workerIndex) {
    uint32_t observedGeneration = workerGeneration.load(std::memory_order_acquire);
    while (!workerStop.load(std::memory_order_acquire)) {
        JobContext context{};
        uint32_t totalWorkers = 0;
        {
            std::unique_lock<std::mutex> lock(workerMutex);
            workerCv.wait(lock, [&observedGeneration] {
                return workerStop.load(std::memory_order_acquire) ||
                    workerGeneration.load(std::memory_order_acquire) != observedGeneration;
            });
            if (workerStop.load(std::memory_order_acquire))
                return;

            observedGeneration = workerGeneration.load(std::memory_order_acquire);
            context = activeJob;
            totalWorkers = workerCount.load(std::memory_order_acquire);
        }
        if (context.type == JobType::Reachability)
            ProcessReachabilityWorker(workerIndex, totalWorkers, context);
        else if (context.type == JobType::SnapshotRollback)
            ProcessSnapshotWorker(workerIndex, totalWorkers, context);

        workerCompleted.fetch_add(1, std::memory_order_release);
        workerCv.notify_all();
    }
}

void CPhysicsAligner::ProcessReachabilityWorker(uint32_t workerIndex, uint32_t totalWorkers, const JobContext& context) {
    if (!context.cache || context.cache->entityCount == 0)
        return;

    const size_t jobs = context.cache->entityCount;
    const size_t chunkSize = (jobs + totalWorkers - 1) / totalWorkers;
    const size_t begin = workerIndex * chunkSize;
    const size_t end = (std::min)(jobs, begin + chunkSize);
    if (begin >= end)
        return;

    SoAEntityCache& cache = *context.cache;
    for (size_t i = begin; i < end; ++i) {
        if (!cache.isActive[i] || !cache.onScreen[i])
            continue;

        const Vector3 targetPos(cache.headX[i], cache.headY[i], cache.headZ[i]);
        const Vector3 delta = targetPos - context.eyePos;
        const Vector3 dir = NormalizeVectorFast(delta);
        const float approxTravel = (delta.m_x * dir.m_x) + (delta.m_y * dir.m_y) + (delta.m_z * dir.m_z);
        const bool reachable = (approxTravel > 0.0f) && (!config.occlusionTest || cache.isVisible[i]);
        raycastResults.TryPush({ RaycastResult::Kind::Raycast, static_cast<uint16_t>(i), reachable });
    }
}

void CPhysicsAligner::ProcessSnapshotWorker(uint32_t workerIndex, uint32_t totalWorkers, const JobContext& context) {
    if (!context.snapshots || context.snapshotCount == 0)
        return;

    const size_t chunkSize = (context.snapshotCount + totalWorkers - 1) / totalWorkers;
    const size_t begin = workerIndex * chunkSize;
    const size_t end = (std::min)(context.snapshotCount, begin + chunkSize);
    if (begin >= end)
        return;

    const __m512 m0 = _mm512_set1_ps(g_ViewMatrixSnapshot.m_data[0][0]); const __m512 m1 = _mm512_set1_ps(g_ViewMatrixSnapshot.m_data[0][1]);
    const __m512 m2 = _mm512_set1_ps(g_ViewMatrixSnapshot.m_data[0][2]); const __m512 m3 = _mm512_set1_ps(g_ViewMatrixSnapshot.m_data[0][3]);
    const __m512 m4 = _mm512_set1_ps(g_ViewMatrixSnapshot.m_data[1][0]); const __m512 m5 = _mm512_set1_ps(g_ViewMatrixSnapshot.m_data[1][1]);
    const __m512 m6 = _mm512_set1_ps(g_ViewMatrixSnapshot.m_data[1][2]); const __m512 m7 = _mm512_set1_ps(g_ViewMatrixSnapshot.m_data[1][3]);
    const __m512 m12 = _mm512_set1_ps(g_ViewMatrixSnapshot.m_data[3][0]); const __m512 m13 = _mm512_set1_ps(g_ViewMatrixSnapshot.m_data[3][1]);
    const __m512 m14 = _mm512_set1_ps(g_ViewMatrixSnapshot.m_data[3][2]); const __m512 m15 = _mm512_set1_ps(g_ViewMatrixSnapshot.m_data[3][3]);
    const __m512 vCenterX = _mm512_set1_ps(context.centerX);
    const __m512 vCenterY = _mm512_set1_ps(context.centerY);
    const __m512 vSensorX = _mm512_set1_ps(context.eyePos.m_x);
    const __m512 vSensorY = _mm512_set1_ps(context.eyePos.m_y);
    const __m512 vSensorZ = _mm512_set1_ps(context.eyePos.m_z);

    for (size_t snapIdx = begin; snapIdx < end; ++snapIdx) {
        const SnapshotState& snapshot = context.snapshots[snapIdx];
        if (snapshot.captureTimeUs == 0 || snapshot.entityCount == 0)
            continue;

        float localBestDistance = std::numeric_limits<float>::max();
        uint16_t localBestEntity = 0;

        for (size_t i = 0; i < snapshot.entityCount; i += 16) {
            const size_t remaining = snapshot.entityCount - i;
            const __mmask16 laneMask = static_cast<__mmask16>((remaining >= 16) ? 0xFFFFu : ((1u << remaining) - 1u));

            __m512 x = _mm512_sub_ps(_mm512_loadu_ps(&snapshot.headX[i]), vSensorX);
            __m512 y = _mm512_sub_ps(_mm512_loadu_ps(&snapshot.headY[i]), vSensorY);
            __m512 z = _mm512_sub_ps(_mm512_loadu_ps(&snapshot.headZ[i]), vSensorZ);

            __m512 w = _mm512_fmadd_ps(x, m12, _mm512_fmadd_ps(y, m13, _mm512_fmadd_ps(z, m14, m15)));
            __mmask16 frontMask = _mm512_mask_cmp_ps_mask(laneMask, w, _mm512_set1_ps(0.01f), _CMP_GT_OS);
            if (!frontMask)
                continue;

            __m512 invW = _mm512_maskz_div_ps(frontMask, _mm512_set1_ps(1.0f), w);
            __m512 projX = _mm512_fmadd_ps(x, m0, _mm512_fmadd_ps(y, m1, _mm512_fmadd_ps(z, m2, m3)));
            projX = _mm512_fmadd_ps(_mm512_mul_ps(projX, invW), vCenterX, vCenterX);
            __m512 projY = _mm512_fmadd_ps(x, m4, _mm512_fmadd_ps(y, m5, _mm512_fmadd_ps(z, m6, m7)));
            projY = _mm512_fnmadd_ps(_mm512_mul_ps(projY, invW), vCenterY, vCenterY);

            alignas(64) float px[16];
            alignas(64) float py[16];
            _mm512_store_ps(px, projX);
            _mm512_store_ps(py, projY);

            for (size_t lane = 0; lane < remaining && lane < 16; ++lane) {
                const size_t idx = i + lane;
                if (!(frontMask & (1u << lane)))
                    continue;
                if (!snapshot.isActive[idx] || !snapshot.isVisible[idx])
                    continue;
                const float dx = px[lane] - context.centerX;
                const float dy = py[lane] - context.centerY;
                const float distance = std::sqrt((dx * dx) + (dy * dy));
                if (distance < localBestDistance && distance < context.laneLimit) {
                    localBestDistance = distance;
                    localBestEntity = static_cast<uint16_t>(idx);
                }
            }
        }

        if (localBestDistance < std::numeric_limits<float>::max()) {
            raycastResults.TryPush({
                RaycastResult::Kind::Snapshot,
                localBestEntity,
                true,
                static_cast<uint16_t>(snapIdx),
                localBestDistance,
                Vector3(snapshot.headX[localBestEntity], snapshot.headY[localBestEntity], snapshot.headZ[localBestEntity])
            });
        }
    }
}

void CPhysicsAligner::ProjectCoordinatesToGrid_AVX512(SoAEntityCache& cache, const VMatrix& viewMatrix, const Vector3& sensorPos) {
    if (cache.entityCount == 0) return;

    __m512 m0 = _mm512_set1_ps(viewMatrix.m_data[0][0]); __m512 m1 = _mm512_set1_ps(viewMatrix.m_data[0][1]);
    __m512 m2 = _mm512_set1_ps(viewMatrix.m_data[0][2]); __m512 m3 = _mm512_set1_ps(viewMatrix.m_data[0][3]);
    __m512 m4 = _mm512_set1_ps(viewMatrix.m_data[1][0]); __m512 m5 = _mm512_set1_ps(viewMatrix.m_data[1][1]);
    __m512 m6 = _mm512_set1_ps(viewMatrix.m_data[1][2]); __m512 m7 = _mm512_set1_ps(viewMatrix.m_data[1][3]);
    __m512 m12 = _mm512_set1_ps(viewMatrix.m_data[3][0]); __m512 m13 = _mm512_set1_ps(viewMatrix.m_data[3][1]);
    __m512 m14 = _mm512_set1_ps(viewMatrix.m_data[3][2]); __m512 m15 = _mm512_set1_ps(viewMatrix.m_data[3][3]);

    if (ioState.screenWidth <= 0 || ioState.screenHeight <= 0) return;

    __m512 centerX = _mm512_set1_ps(ioState.screenWidth * 0.5f);
    __m512 centerY = _mm512_set1_ps(ioState.screenHeight * 0.5f);

    const size_t simdStride = 16;
    const __m512 sensorX = _mm512_set1_ps(sensorPos.m_x);
    const __m512 sensorY = _mm512_set1_ps(sensorPos.m_y);
    const __m512 sensorZ = _mm512_set1_ps(sensorPos.m_z);
    for (size_t i = 0; i < cache.entityCount; i += simdStride) {
        const size_t remaining = cache.entityCount - i;
        const __mmask16 laneMask = static_cast<__mmask16>((remaining >= simdStride) ? 0xFFFFu : ((1u << remaining) - 1u));
        __m512 x = _mm512_sub_ps(_mm512_loadu_ps(&cache.headX[i]), sensorX);
        __m512 y = _mm512_sub_ps(_mm512_loadu_ps(&cache.headY[i]), sensorY);
        __m512 z = _mm512_sub_ps(_mm512_loadu_ps(&cache.headZ[i]), sensorZ);

        __m512 w = _mm512_fmadd_ps(x, m12, _mm512_fmadd_ps(y, m13, _mm512_fmadd_ps(z, m14, m15)));
        __mmask16 frontMask = _mm512_mask_cmp_ps_mask(laneMask, w, _mm512_set1_ps(0.01f), _CMP_GT_OS);
        
        if (frontMask == 0) {
            for (size_t j = 0; j < remaining && j < simdStride; ++j) cache.onScreen[i + j] = false;
            continue;
        }

        __m512 invW = _mm512_maskz_div_ps(frontMask, _mm512_set1_ps(1.0f), w);
        __m512 projX = _mm512_fmadd_ps(x, m0, _mm512_fmadd_ps(y, m1, _mm512_fmadd_ps(z, m2, m3)));
        projX = _mm512_fmadd_ps(_mm512_mul_ps(projX, invW), centerX, centerX);
        __m512 projY = _mm512_fmadd_ps(x, m4, _mm512_fmadd_ps(y, m5, _mm512_fmadd_ps(z, m6, m7)));
        projY = _mm512_fnmadd_ps(_mm512_mul_ps(projY, invW), centerY, centerY);

        _mm512_storeu_ps(&cache.screenX[i], projX);
        _mm512_storeu_ps(&cache.screenY[i], projY);
        for (size_t j = 0; j < remaining && j < simdStride; ++j) cache.onScreen[i + j] = (frontMask & (1u << j)) != 0;
    }
}

void CPhysicsAligner::ScanObjectCluster() {
    auto& localCache = ThreadLocalStaging::localCache;
    auto& localArena = ThreadLocalStaging::localArena;
    localArena.Reset();
    localCache.Clear();
    auto* pEngine = SDK::Interfaces::EngineToClient();
    if (!pEngine || !pEngine->IsInGame()) return;
    auto* pLocalController = GetCL_Players()->GetLocalPlayerController();
    if (!pLocalController) return;
    int localTeam = pLocalController->m_iTeamNum();
    auto* pCache = GetEntityCache();
    using CachedEntityVec = std::remove_reference_t<decltype(*pCache->GetCachedEntity())>;
    CachedEntityVec snapshot;
    {
        std::scoped_lock lock(pCache->GetLock());
        const auto& cachedVec = pCache->GetCachedEntity();
        snapshot = *cachedVec;
    }

    const uint64_t nowUs = GetTimestampUs();
    for (const auto& cachedEntity : snapshot) {
        auto* pController = reinterpret_cast<CCSPlayerController*>(cachedEntity.m_Handle.Get());
        if (!pController || !pController->m_bPawnIsAlive() || pController->m_iTeamNum() == localTeam) continue;
        auto* pPawn = pController->m_hPawn().Get<C_CSPlayerPawn>();
        if (!pPawn) continue;
        Vector3 headPos = GetCL_Bones()->GetBonePositionByName(pPawn, "head_0");
        if (headPos.IsZero()) continue;
        bool isVisible = config.occlusionTest ? SampleVisibilityCached(pPawn, nowUs, config.visibilityCacheMs) : true;
        localCache.AddEntity(headPos, pController->m_iTeamNum(), (float)pPawn->m_iHealth(), isVisible);
    }
}

__m512 CPhysicsAligner::fast_rsqrt14_ps(__m512 v) {
    return _mm512_rsqrt14_ps(v);
}

Vector3 CPhysicsAligner::NormalizeVectorFast(const Vector3& v) {
    const __m512 x = _mm512_set1_ps(v.m_x);
    const __m512 y = _mm512_set1_ps(v.m_y);
    const __m512 z = _mm512_set1_ps(v.m_z);
    const __m512 lenSq = _mm512_fmadd_ps(x, x, _mm512_fmadd_ps(y, y, _mm512_mul_ps(z, z)));
    const __m512 invLen = fast_rsqrt14_ps(_mm512_max_ps(lenSq, _mm512_set1_ps(1.0e-12f)));
    const __m512 nx = _mm512_mul_ps(x, invLen);
    const __m512 ny = _mm512_mul_ps(y, invLen);
    const __m512 nz = _mm512_mul_ps(z, invLen);

    return Vector3(
        _mm512_cvtss_f32(nx),
        _mm512_cvtss_f32(ny),
        _mm512_cvtss_f32(nz)
    );
}

void CPhysicsAligner::ResolvePhaseBypassReachability(SoAEntityCache& cache, const Vector3& eyePos) {
    if (!config.phaseBypass || cache.entityCount == 0)
        return;

    raycastResults.Reset();

    activeJob.cache = &cache;
    activeJob.eyePos = eyePos;
    DispatchParallelJob(JobType::Reachability);

    RaycastResult result{};
    while (raycastResults.TryPop(result)) {
        if (result.kind == RaycastResult::Kind::Raycast && result.entityIndex < cache.entityCount)
            cache.isVisible[result.entityIndex] = result.reachable;
    }
}

uint64_t CPhysicsAligner::GetTimestampUs() {
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(now).count());
}

void CPhysicsAligner::CaptureDeterministicSnapshot(const SoAEntityCache& cache, uint64_t nowUs) {
    auto& history = ThreadLocalStaging::snapshotHistory;
    auto& writeIndex = ThreadLocalStaging::snapshotWriteIndex;
    auto& snapshotCount = ThreadLocalStaging::snapshotCount;

    SnapshotState& slot = history[writeIndex];
    slot.captureTimeUs = nowUs;
    const size_t usedCount = (std::min)(cache.entityCount, SoAEntityCache::MAX_ENTITIES);
    slot.entityCount = usedCount;

    if (usedCount > 0) {
        memcpy(slot.headX, cache.headX, sizeof(float) * usedCount);
        memcpy(slot.headY, cache.headY, sizeof(float) * usedCount);
        memcpy(slot.headZ, cache.headZ, sizeof(float) * usedCount);
        memcpy(slot.screenX, cache.screenX, sizeof(float) * usedCount);
        memcpy(slot.screenY, cache.screenY, sizeof(float) * usedCount);
        memcpy(slot.isActive, cache.isActive, sizeof(bool) * usedCount);
        memcpy(slot.isVisible, cache.isVisible, sizeof(bool) * usedCount);
        memcpy(slot.onScreen, cache.onScreen, sizeof(bool) * usedCount);
    }

    writeIndex = (writeIndex + 1) % SnapshotState::MAX_HISTORY;
    snapshotCount = (std::min)(snapshotCount + 1, SnapshotState::MAX_HISTORY);
}

void CPhysicsAligner::InvalidateStaleSnapshots(uint64_t nowUs, uint64_t staleWindowUs) {
    auto& history = ThreadLocalStaging::snapshotHistory;
    auto& snapshotCount = ThreadLocalStaging::snapshotCount;
    for (size_t i = 0; i < snapshotCount; ++i) {
        SnapshotState& snapshot = history[i];
        if (snapshot.captureTimeUs == 0 || (nowUs - snapshot.captureTimeUs) > staleWindowUs) {
            snapshot.captureTimeUs = 0;
            snapshot.entityCount = 0;
        }
    }
}

bool CPhysicsAligner::ResolveTemporalRollbackTarget(const Vector3& eyePos, float& inOutBestDistance, Vector3& outTargetPos) {
    auto& history = ThreadLocalStaging::snapshotHistory;
    auto& snapshotCount = ThreadLocalStaging::snapshotCount;
    if (!config.backtrackEnabled || snapshotCount == 0 || ioState.screenWidth <= 0.0f || ioState.screenHeight <= 0.0f)
        return false;

    activeJob.centerX = ioState.screenWidth * 0.5f;
    activeJob.centerY = ioState.screenHeight * 0.5f;
    activeJob.laneLimit = config.fov * 10.0f;
    activeJob.eyePos = eyePos;
    activeJob.snapshots = history;
    activeJob.snapshotCount = snapshotCount;

    raycastResults.Reset();
    DispatchParallelJob(JobType::SnapshotRollback);

    bool found = false;
    RaycastResult result{};
    while (raycastResults.TryPop(result)) {
        if (result.kind != RaycastResult::Kind::Snapshot || !result.reachable)
            continue;
        if (result.score < inOutBestDistance) {
            inOutBestDistance = result.score;
            outTargetPos = result.snapshotPosition;
            found = true;
        }
    }

    (void)eyePos;
    return found;
}

void CPhysicsAligner::SolveConstraint(Vector3& opticalOrientation, bool& triggerInteraction) {
    if (!config.enabled) return;

    auto& cache = ThreadLocalStaging::localCache;
    auto& localArena = ThreadLocalStaging::localArena;
    if (cache.entityCount == 0) return;

    auto* pLocalPawn = GetCL_Players()->GetLocalPlayerPawn();
    if (!pLocalPawn) return;

    const Vector3 eyePos = pLocalPawn->m_vOldOrigin() + pLocalPawn->m_vecViewOffset();
    ProjectCoordinatesToGrid_AVX512(cache, g_ViewMatrixSnapshot, eyePos);
    ResolvePhaseBypassReachability(cache, eyePos);
    const uint64_t nowUs = GetTimestampUs();
    if (config.backtrackEnabled) {
        InvalidateStaleSnapshots(nowUs, 200000ULL);
        CaptureDeterministicSnapshot(cache, nowUs);
    }

    if (ioState.screenWidth <= 0.0f || ioState.screenHeight <= 0.0f)
        return;

    const __m512 centerX = _mm512_set1_ps(ioState.screenWidth * 0.5f);
    const __m512 centerY = _mm512_set1_ps(ioState.screenHeight * 0.5f);
    const __m512 laneLimit = _mm512_set1_ps(config.fov * 10.0f);

    FireCommand& best = ThreadLocalStaging::pendingCommand;
    best.shouldFire = false;
    best.fov = config.fov;

    float bestDistance = std::numeric_limits<float>::max();
    size_t bestIndex = 0;
    constexpr size_t stride = 16;
    float* distanceScratch = static_cast<float*>(localArena.Alloc(sizeof(float) * cache.entityCount));
    if (distanceScratch)
        memset(distanceScratch, 0, sizeof(float) * cache.entityCount);

    for (size_t i = 0; i < cache.entityCount; i += stride) {
        const size_t remaining = cache.entityCount - i;
        const __mmask16 laneMask = static_cast<__mmask16>((remaining >= stride) ? 0xFFFFu : ((1u << remaining) - 1u));

        __m512 sx = _mm512_loadu_ps(&cache.screenX[i]);
        __m512 sy = _mm512_loadu_ps(&cache.screenY[i]);
        __m512 dx = _mm512_sub_ps(sx, centerX);
        __m512 dy = _mm512_sub_ps(sy, centerY);
        __m512 distSq = _mm512_fmadd_ps(dx, dx, _mm512_mul_ps(dy, dy));
        __m512 invDist = fast_rsqrt14_ps(_mm512_max_ps(distSq, _mm512_set1_ps(1.0e-12f)));
        __m512 dist = _mm512_mul_ps(distSq, invDist);

        __mmask16 validMask = laneMask;
        for (size_t lane = 0; lane < remaining && lane < stride; ++lane) {
            const size_t idx = i + lane;
            const bool visible = !config.occlusionTest || cache.isVisible[idx];
            if (!(cache.isActive[idx] && cache.onScreen[idx] && visible))
                validMask &= static_cast<__mmask16>(~(1u << lane));
        }
        validMask &= _mm512_mask_cmp_ps_mask(validMask, dist, laneLimit, _CMP_LT_OS);

        while (validMask) {
            uint32_t lane = 0;
            while (lane < stride && ((validMask & (1u << lane)) == 0))
                ++lane;
            if (lane >= stride)
                break;
            validMask &= static_cast<__mmask16>(~(1u << lane));

            const size_t idx = i + lane;
            const float preciseDx = cache.screenX[idx] - (ioState.screenWidth * 0.5f);
            const float preciseDy = cache.screenY[idx] - (ioState.screenHeight * 0.5f);
            const float preciseDistance = std::sqrt((preciseDx * preciseDx) + (preciseDy * preciseDy));
            if (distanceScratch)
                distanceScratch[idx] = preciseDistance;

            if (preciseDistance < bestDistance) {
                bestDistance = preciseDistance;
                bestIndex = idx;
                best.shouldFire = true;
            }
        }
    }

    Vector3 temporalTargetPos{0, 0, 0};
    if (config.backtrackEnabled && ResolveTemporalRollbackTarget(eyePos, bestDistance, temporalTargetPos)) {
        best.shouldFire = true;
        bestIndex = SoAEntityCache::MAX_ENTITIES; // sentinel for temporal path
    }

    if (!best.shouldFire)
        return;

    const Vector3 targetPos = (bestIndex == SoAEntityCache::MAX_ENTITIES)
        ? temporalTargetPos
        : Vector3(cache.headX[bestIndex], cache.headY[bestIndex], cache.headZ[bestIndex]);
    QAngle qAngle = Math::CalcAngle(eyePos, targetPos);
    best.targetAngle = Vector3(qAngle.m_x, qAngle.m_y, qAngle.m_z);
    best.fov = bestDistance * 0.1f;

    Vector3 finalAngle = best.targetAngle;
    if (config.vibrationDamping) {
        const QAngle punch = pLocalPawn->m_aimPunchCache()[0];
        const __m512 orientation = _mm512_setr_ps(finalAngle.m_x, finalAngle.m_y, finalAngle.m_z, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        const __m512 vibration = _mm512_setr_ps(punch.m_x, punch.m_y, 0.0f, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        const __m512 dampingScale = _mm512_setr_ps(-config.vibrationDampingX, -config.vibrationDampingY, 0.0f, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        const __m512 damped = _mm512_fmadd_ps(vibration, dampingScale, orientation);
        alignas(64) float dampedValues[16];
        _mm512_store_ps(dampedValues, damped);
        finalAngle.m_x = dampedValues[0];
        finalAngle.m_y = dampedValues[1];
    }

    if (config.smooth > 1.0f) {
        Vector3 delta = finalAngle - opticalOrientation;
        while (delta.m_x > 180.0f) delta.m_x -= 360.0f;
        while (delta.m_x < -180.0f) delta.m_x += 360.0f;
        while (delta.m_y > 180.0f) delta.m_y -= 360.0f;
        while (delta.m_y < -180.0f) delta.m_y += 360.0f;
        opticalOrientation = opticalOrientation + (delta / config.smooth);
    } else {
        opticalOrientation = finalAngle;
    }

    if (best.fov < 1.0f)
        triggerInteraction = true;
}
