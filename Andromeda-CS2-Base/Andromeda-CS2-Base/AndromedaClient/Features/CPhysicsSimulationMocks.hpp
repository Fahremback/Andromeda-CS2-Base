#pragma once

#include "CPhysicsCriticalPhases.hpp"
#include <cstddef>
#include <cstdint>

class CPhysicsSimulationMocks
{
public:
    static constexpr uint32_t FRAME_RENDER_START_MOCK = 5;

    struct alignas(64) UserCmdLayout
    {
        size_t csgoUserCmdOffset = 0x08;
        size_t baseCmdPtrOffset = 0x10;
        size_t viewAnglesOffset = 0x18;
    };

    struct alignas(64) PawnRenderLayout
    {
        size_t eyeAnglesOffset = 0x40;
    };

    struct alignas(64) NetworkOutputLayout
    {
        size_t telemetryBufferOffset = 0x80;
        size_t telemetryCapacityBytes = 64 * 1024;
    };

    static bool ApplyCollisionSnapshot(void* pUserCmdBase,
                                       size_t userCmdSize,
                                       const UserCmdLayout& layout,
                                       const CPhysicsCriticalPhases::SensorMatrixState& ioState);

    static bool ApplyRenderSnapshot(void* pPawnBase,
                                    size_t pawnSize,
                                    const PawnRenderLayout& layout,
                                    uint32_t frameStage,
                                    const CPhysicsCriticalPhases::SensorMatrixState& ioState);

    static size_t FlushTelemetryToNetworkMock(void* pClientInputBase,
                                              size_t clientInputSize,
                                              const NetworkOutputLayout& layout,
                                              CPhysicsCriticalPhases::TelemetryRingBuffer& queue,
                                              uint32_t omissionMask = CPhysicsCriticalPhases::Phase5::IN_ATTACK);
};
