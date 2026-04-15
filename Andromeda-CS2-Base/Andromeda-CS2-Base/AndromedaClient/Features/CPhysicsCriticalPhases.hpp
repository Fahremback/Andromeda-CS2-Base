#pragma once

#include "../../CS2/SDK/Math/Vector3.hpp"
#include <cstdint>
#include <cstddef>

class CPhysicsCriticalPhases
{
public:
    struct alignas(64) PenetrationRay
    {
        Vector3 start{ 0, 0, 0 };
        Vector3 direction{ 0, 0, 1 };
        float maxDistance = 0.0f;
    };

    struct alignas(64) MaterialLayer
    {
        float entryDistance = 0.0f;
        float exitDistance = 0.0f;
        float density = 1.0f;
        float resistance = 1.0f;
        uint32_t flags = 0;
    };

    struct alignas(64) PenetrationResult
    {
        float remainingEnergy = 0.0f;
        float accumulatedThickness = 0.0f;
        size_t processedLayers = 0;
        bool reachedTarget = false;
    };

    struct alignas(64) SensorMatrixState
    {
        Vector3 collisionAngles{ 0, 0, 0 };
        Vector3 renderAngles{ 0, 0, 0 };
        Vector3 collisionOrigin{ 0, 0, 0 };
        Vector3 renderOrigin{ 0, 0, 0 };
    };

    struct alignas(64) KinematicOscillationState
    {
        float lbyAccumulator = 0.0f;
        float zOscillationPhase = 0.0f;
        float lastTickDt = 0.0f;
    };

    struct alignas(64) ExternalEntitySoA
    {
        float* posX = nullptr;
        float* posY = nullptr;
        float* posZ = nullptr;
        float* velX = nullptr;
        float* velY = nullptr;
        float* velZ = nullptr;
        float* baseYaw = nullptr;
        float* lean = nullptr;
        float* moveYaw = nullptr;
        float* collisionYaw = nullptr;
        size_t count = 0;
    };

    struct alignas(64) DecryptionState
    {
        float bruteYaw = 0.0f;
        float overlayYaw = 0.0f;
        float correctedYaw = 0.0f;
        float desyncDelta = 0.0f;
    };

    struct alignas(64) FrameContext
    {
        uint64_t tick = 0;
        uint64_t timestampUs = 0;
        float dt = 0.0f;
    };

    struct alignas(64) TelemetryCmd
    {
        uint64_t tick = 0;
        uint32_t flags = 0;
        float payload[4]{ 0, 0, 0, 0 };
        FrameContext context{};
    };

    struct alignas(64) TelemetryRingBuffer
    {
        static constexpr size_t CAPACITY = 1024;
        alignas(64) TelemetryCmd entries[CAPACITY];
        size_t head = 0;
        size_t tail = 0;
        size_t count = 0;
        uint64_t chokeStartTick = 0;
    };

    class Phase2
    {
    public:
        static bool Volumetric_Penetration_Solver_TraceIO(const PenetrationRay& ray,
                                                          float initialEnergy,
                                                          float minimumThreshold,
                                                          uint32_t maxImpacts,
                                                          PenetrationResult& outResult);

        static bool Volumetric_Penetration_Solver(const PenetrationRay& ray,
                                                  const MaterialLayer* layers,
                                                  size_t layerCount,
                                                  float initialEnergy,
                                                  float minimumThreshold,
                                                  PenetrationResult& outResult);

        static float Material_Density_Energy_Loss(float energyIn, float density, float thickness, float resistance);
        static bool Minimum_Kinetic_Threshold(float remainingEnergy, float minimumThreshold);
        static bool Non_Linear_Phase_Bypass(uint32_t layerFlags);
        static float Bisection_Thickness_Check(float entryDistance, float exitDistance, uint32_t maxIterations);
    };

    class Phase4
    {
    public:
        static void Asynchronous_Mesh_Decoupling(SensorMatrixState& ioState, float decouplingStrength, float dt);
        static void Zenith_Nadir_Gimbal_Lock(SensorMatrixState& ioState, bool lockToZenith);
        static void Stochastic_Yaw_Dispersion(SensorMatrixState& ioState, uint32_t& rngState, float jitterAmplitude);
        static void Lower_Body_Kinematic_Break(SensorMatrixState& ioState, KinematicOscillationState& ioKinematics, float dt, float breakRateDeg);
        static void High_Frequency_Z_Axis_Oscillation(SensorMatrixState& ioState, KinematicOscillationState& ioKinematics, float amplitude, float frequencyHz, float dt);
        static void Micro_Movement_State_Preservation(SensorMatrixState& ioState, float epsilonStep);
        static void Inertial_Damping_Override(Vector3& ioVelocity);
    };

    class Phase6
    {
    public:
        static DecryptionState Heuristic_Angle_Decryption(const ExternalEntitySoA& entities, size_t entityIndex, float referenceYaw);
        static float Animation_Overlay_Synchronization(float baseYaw, float leanLayer, float moveYawLayer);
        static float Asymmetrical_Desync_Correction(float bruteYaw, float overlayYaw, float leanLayerWeight);
        static void Predictive_State_Extrapolation(ExternalEntitySoA& entities, float missingDeltaTime, float gravityAcceleration);
    };

    class Phase5
    {
    public:
        static constexpr uint32_t IN_ATTACK = ( 1u << 0 );
        static size_t Telemetry_Batch_Execution(TelemetryRingBuffer& queue, TelemetryCmd* outBatch, size_t outCapacity);
        static bool Packet_Accumulation_Choke(TelemetryRingBuffer& queue, const TelemetryCmd& cmd, uint32_t holdTicks, uint64_t currentTick);
        static void Command_Sequence_Omission(TelemetryRingBuffer& queue, uint32_t omissionMask);
        static void Artificial_Latency_Padding(TelemetryRingBuffer& queue, uint64_t latencyUs);
        static void Time_Delta_Compression(TelemetryRingBuffer& queue, uint32_t subSteps, float compressionFactor);
    };
};
