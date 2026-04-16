#include "CPhysicsSimulationMocks.hpp"

#include <cstring>
#include <cstdint>

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
    // Validate that the view angles address is within a reasonable range
    // and not pointing to kernel space or obviously invalid addresses
    if (viewAnglesAddr < 0x1000)
        return false;

    // Simple non-atomic write - safer during match transitions where atomic_ref
    // can race with game's own access to the same memory
    auto* pViewAngles = reinterpret_cast<volatile Vector3*>(viewAnglesAddr);
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

    // Validate pawn size is reasonable (C_CSPlayerPawn is typically 0x3000-0x5000 bytes)
    if (pawnSize < 0x1000 || pawnSize > 0x10000)
        return false;

    const uintptr_t pawnBaseAddr = reinterpret_cast<uintptr_t>(pPawnBase);
    const uintptr_t eyeAnglesAddr = pawnBaseAddr + layout.eyeAnglesOffset;
    if ((eyeAnglesAddr + sizeof(Vector3)) > (pawnBaseAddr + pawnSize))
        return false;

    // Use volatile plain write instead of atomic_ref - avoids UB from
    // unaligned access and race conditions during pawn construction/destruction
    auto* pEyeAngles = reinterpret_cast<volatile Vector3*>(eyeAnglesAddr);
    pEyeAngles->m_x = ioState.renderAngles.m_x;
    pEyeAngles->m_y = ioState.renderAngles.m_y;
    pEyeAngles->m_z = ioState.renderAngles.m_z;
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
