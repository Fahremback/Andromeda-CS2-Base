
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
#include <future>
#include <thread>
#include <vector>
#include <limits>
#include <memory>
#include <type_traits>
#include <malloc.h>
#include <chrono>

thread_local CPhysicsAligner::SoAEntityCache CPhysicsAligner::ThreadLocalStaging::localCache;
thread_local CPhysicsAligner::FireCommand CPhysicsAligner::ThreadLocalStaging::pendingCommand;
thread_local CPhysicsAligner::ArenaAllocator CPhysicsAligner::ThreadLocalStaging::localArena;
thread_local CPhysicsAligner::SnapshotState CPhysicsAligner::ThreadLocalStaging::snapshotHistory[CPhysicsAligner::SnapshotState::MAX_HISTORY];
thread_local size_t CPhysicsAligner::ThreadLocalStaging::snapshotWriteIndex = 0;
thread_local size_t CPhysicsAligner::ThreadLocalStaging::snapshotCount = 0;

namespace
{
    constexpr uint32_t kMaxWorkers = 16;
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

void CPhysicsAligner::Initialize() {
    config = BallSimulationParams{};
}

void CPhysicsAligner::ProjectCoordinatesToGrid_AVX512(SoAEntityCache& cache, const VMatrix& viewMatrix, const Vector3& sensorPos) {
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

    for (const auto& cachedEntity : snapshot) {
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

    const size_t jobs = cache.entityCount;
    const uint32_t workerCount = (std::min<uint32_t>)((std::max)(1, GetOptimalThreadCount()), kMaxWorkers);
    const size_t chunkSize = (jobs + workerCount - 1) / workerCount;
    std::vector<std::future<void>> workers;
    workers.reserve(workerCount);

    for (uint32_t worker = 0; worker < workerCount; ++worker) {
        const size_t begin = worker * chunkSize;
        const size_t end = (std::min)(jobs, begin + chunkSize);
        if (begin >= end)
            continue;

        workers.emplace_back(std::async(std::launch::async, [&cache, &eyePos, begin, end]() {
            for (size_t i = begin; i < end; ++i) {
                if (!cache.isActive[i] || !cache.onScreen[i])
                    continue;

                const Vector3 targetPos(cache.headX[i], cache.headY[i], cache.headZ[i]);
                const Vector3 delta = targetPos - eyePos;
                const Vector3 dir = NormalizeVectorFast(delta);
                const float approxTravel = (delta.m_x * dir.m_x) + (delta.m_y * dir.m_y) + (delta.m_z * dir.m_z);
                const bool reachable = (approxTravel > 0.0f) && (!config.occlusionTest || cache.isVisible[i]);

                raycastResults.TryPush({ RaycastResult::Kind::Raycast, static_cast<uint16_t>(i), reachable });
            }
        }));
    }

    for (auto& worker : workers)
        worker.wait();

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
    slot.entityCount = cache.entityCount;

    if (cache.entityCount > 0) {
        memcpy(slot.headX, cache.headX, sizeof(float) * cache.entityCount);
        memcpy(slot.headY, cache.headY, sizeof(float) * cache.entityCount);
        memcpy(slot.headZ, cache.headZ, sizeof(float) * cache.entityCount);
        memcpy(slot.screenX, cache.screenX, sizeof(float) * cache.entityCount);
        memcpy(slot.screenY, cache.screenY, sizeof(float) * cache.entityCount);
        memcpy(slot.isActive, cache.isActive, sizeof(bool) * cache.entityCount);
        memcpy(slot.isVisible, cache.isVisible, sizeof(bool) * cache.entityCount);
        memcpy(slot.onScreen, cache.onScreen, sizeof(bool) * cache.entityCount);
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
    if (snapshotCount == 0)
        return false;

    const ImVec2 screenSize = ImGui::GetIO().DisplaySize;
    const float centerX = screenSize.x * 0.5f;
    const float centerY = screenSize.y * 0.5f;
    const float laneLimit = config.fov * 10.0f;

    raycastResults.Reset();

    const uint32_t workerCount = (std::min<uint32_t>)((std::max)(1, GetOptimalThreadCount()), kMaxWorkers);
    std::vector<std::future<void>> workers;
    workers.reserve(workerCount);

    const size_t chunkSize = (snapshotCount + workerCount - 1) / workerCount;
    for (uint32_t worker = 0; worker < workerCount; ++worker) {
        const size_t begin = worker * chunkSize;
        const size_t end = (std::min)(snapshotCount, begin + chunkSize);
        if (begin >= end)
            continue;

        workers.emplace_back(std::async(std::launch::async, [begin, end, &history, centerX, centerY, laneLimit]() {
            for (size_t snapIdx = begin; snapIdx < end; ++snapIdx) {
                const SnapshotState& snapshot = history[snapIdx];
                if (snapshot.captureTimeUs == 0 || snapshot.entityCount == 0)
                    continue;

                float localBestDistance = std::numeric_limits<float>::max();
                uint16_t localBestEntity = 0;
                for (size_t i = 0; i < snapshot.entityCount; ++i) {
                    if (!snapshot.isActive[i] || !snapshot.onScreen[i] || !snapshot.isVisible[i])
                        continue;
                    const float dx = snapshot.screenX[i] - centerX;
                    const float dy = snapshot.screenY[i] - centerY;
                    const float distance = std::sqrt((dx * dx) + (dy * dy));
                    if (distance < localBestDistance && distance < laneLimit) {
                        localBestDistance = distance;
                        localBestEntity = static_cast<uint16_t>(i);
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
        }));
    }

    for (auto& worker : workers)
        worker.wait();

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
    ProjectCoordinatesToGrid_AVX512(cache, g_ViewMatrix, eyePos);
    ResolvePhaseBypassReachability(cache, eyePos);
    const uint64_t nowUs = GetTimestampUs();
    InvalidateStaleSnapshots(nowUs, 200000ULL);
    CaptureDeterministicSnapshot(cache, nowUs);

    const ImVec2 screenSize = ImGui::GetIO().DisplaySize;
    const __m512 centerX = _mm512_set1_ps(screenSize.x * 0.5f);
    const __m512 centerY = _mm512_set1_ps(screenSize.y * 0.5f);
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
            const float preciseDx = cache.screenX[idx] - (screenSize.x * 0.5f);
            const float preciseDy = cache.screenY[idx] - (screenSize.y * 0.5f);
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
    if (ResolveTemporalRollbackTarget(eyePos, bestDistance, temporalTargetPos)) {
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
        if (delta.m_x > 180.0f) delta.m_x -= 360.0f;
        if (delta.m_x < -180.0f) delta.m_x += 360.0f;
        opticalOrientation = opticalOrientation + (delta / config.smooth);
    } else {
        opticalOrientation = finalAngle;
    }

    if (best.fov < 1.0f)
        triggerInteraction = true;
}
