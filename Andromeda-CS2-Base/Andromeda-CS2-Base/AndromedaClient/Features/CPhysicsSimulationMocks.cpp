#include "CPhysicsSimulationMocks.hpp"

#include <atomic>
#include <cstring>

bool CPhysicsSimulationMocks::ApplyCollisionSnapshot(void* pUserCmdBase,
                                                     size_t userCmdSize,
                                                     const UserCmdLayout& layout,
                                                     const CPhysicsCriticalPhases::SensorMatrixState& ioState)
{
    if (!pUserCmdBase || userCmdSize == 0)
        return false;

    const uintptr_t userBase = reinterpret_cast<uintptr_t>(pUserCmdBase);
    const uintptr_t csgoUserCmdAddr = userBase + layout.csgoUserCmdOffset;
    const uintptr_t baseCmdPtrAddr = csgoUserCmdAddr + layout.baseCmdPtrOffset;

    if ((baseCmdPtrAddr + sizeof(uintptr_t)) > (userBase + userCmdSize))
        return false;

    uintptr_t baseCmdAddr = 0;
    std::memcpy(&baseCmdAddr, reinterpret_cast<const void*>(baseCmdPtrAddr), sizeof(uintptr_t));
    if (baseCmdAddr == 0)
        return false;

    const uintptr_t viewAnglesAddr = baseCmdAddr + layout.viewAnglesOffset;
    auto* pViewAngles = reinterpret_cast<Vector3*>(viewAnglesAddr);
    if (!pViewAngles)
        return false;

    pViewAngles->m_x = ioState.collisionAngles.m_x;
    pViewAngles->m_y = ioState.collisionAngles.m_y;
    pViewAngles->m_z = ioState.collisionAngles.m_z;
    return true;
}

bool CPhysicsSimulationMocks::ApplyRenderSnapshot(void* pPawnBase,
                                                  size_t pawnSize,
                                                  const PawnRenderLayout& layout,
                                                  uint32_t frameStage,
                                                  const CPhysicsCriticalPhases::SensorMatrixState& ioState)
{
    if (!pPawnBase || frameStage != FRAME_RENDER_START_MOCK)
        return false;

    const uintptr_t pawnBase = reinterpret_cast<uintptr_t>(pPawnBase);
    const uintptr_t eyeAnglesAddr = pawnBase + layout.eyeAnglesOffset;
    if ((eyeAnglesAddr + sizeof(Vector3)) > (pawnBase + pawnSize))
        return false;

    auto* pEyeAngles = reinterpret_cast<Vector3*>(eyeAnglesAddr);
    std::atomic_ref<float> xRef(pEyeAngles->m_x);
    std::atomic_ref<float> yRef(pEyeAngles->m_y);
    std::atomic_ref<float> zRef(pEyeAngles->m_z);

    xRef.store(ioState.renderAngles.m_x, std::memory_order_release);
    yRef.store(ioState.renderAngles.m_y, std::memory_order_release);
    zRef.store(ioState.renderAngles.m_z, std::memory_order_release);
    return true;
}

size_t CPhysicsSimulationMocks::FlushTelemetryToNetworkMock(void* pClientInputBase,
                                                            size_t clientInputSize,
                                                            const NetworkOutputLayout& layout,
                                                            CPhysicsCriticalPhases::TelemetryRingBuffer& queue,
                                                            uint32_t omissionMask)
{
    if (!pClientInputBase || clientInputSize == 0 || layout.telemetryCapacityBytes == 0)
        return 0;

    CPhysicsCriticalPhases::Phase5::Command_Sequence_Omission(queue, omissionMask);

    const uintptr_t base = reinterpret_cast<uintptr_t>(pClientInputBase);
    const uintptr_t outBufferAddr = base + layout.telemetryBufferOffset;
    if ((outBufferAddr + layout.telemetryCapacityBytes) > (base + clientInputSize))
        return 0;

    auto* outBytes = reinterpret_cast<uint8_t*>(outBufferAddr);
    const size_t maxCmds = layout.telemetryCapacityBytes / sizeof(CPhysicsCriticalPhases::TelemetryCmd);
    if (maxCmds == 0)
        return 0;

    size_t flushed = 0;
    alignas(64) CPhysicsCriticalPhases::TelemetryCmd localBatch[64];
    while (flushed < maxCmds) {
        const size_t batchCap = (std::min)(static_cast<size_t>(64), maxCmds - flushed);
        const size_t pulled = CPhysicsCriticalPhases::Phase5::Telemetry_Batch_Execution(queue, localBatch, batchCap);
        if (pulled == 0)
            break;

        std::memcpy(outBytes + (flushed * sizeof(CPhysicsCriticalPhases::TelemetryCmd)),
                    localBatch,
                    pulled * sizeof(CPhysicsCriticalPhases::TelemetryCmd));
        flushed += pulled;
    }

    return flushed;
}
