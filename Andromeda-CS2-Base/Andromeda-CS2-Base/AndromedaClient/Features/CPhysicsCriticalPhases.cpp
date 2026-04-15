#include "CPhysicsCriticalPhases.hpp"
#include "../../GameClient/CL_Trace.hpp"

#include <immintrin.h>
#include <algorithm>
#include <cmath>

namespace
{
    constexpr uint32_t kLayerFlagDynamic = 1u << 0;
    constexpr float kPitchClamp = 89.0f;
    constexpr float kYawWrap = 180.0f;
    constexpr float kTwoPi = 6.28318530717958647692f;
    constexpr float kDegToRad = 0.01745329251994329577f;
    constexpr float kRadToDeg = 57.295779513082320876f;

    inline float WrapYaw(float yaw)
    {
        while (yaw > 180.0f) yaw -= 360.0f;
        while (yaw < -180.0f) yaw += 360.0f;
        return yaw;
    }
}

bool CPhysicsCriticalPhases::Phase2::Volumetric_Penetration_Solver(const PenetrationRay& ray,
                                                                    const MaterialLayer* layers,
                                                                    size_t layerCount,
                                                                    float initialEnergy,
                                                                    float minimumThreshold,
                                                                    PenetrationResult& outResult)
{
    outResult = {};
    if (!layers || layerCount == 0 || initialEnergy <= 0.0f || ray.maxDistance <= 0.0f)
        return false;

    float remainingEnergy = initialEnergy;
    float accumulatedThickness = 0.0f;
    size_t processedLayers = 0;
    const __m512 vResistanceScale = _mm512_set1_ps(1.0f);

    for (size_t i = 0; i < layerCount; i += 16) {
        const size_t remaining = layerCount - i;
        const size_t batchCount = (std::min)(remaining, static_cast<size_t>(16));

        alignas(64) float densities[16] = {};
        alignas(64) float resistances[16] = {};
        alignas(64) float thicknesses[16] = {};
        alignas(64) float losses[16] = {};

        uint16_t validMask = 0;
        for (size_t lane = 0; lane < batchCount; ++lane) {
            const MaterialLayer& layer = layers[i + lane];
            if (Non_Linear_Phase_Bypass(layer.flags))
                continue;

            const float thickness = Bisection_Thickness_Check(layer.entryDistance, layer.exitDistance, 8);
            if (thickness <= 0.0f)
                continue;

            thicknesses[lane] = thickness;
            densities[lane] = (std::max)(layer.density, 0.0f);
            resistances[lane] = (std::max)(layer.resistance, 0.0f);
            validMask |= static_cast<uint16_t>(1u << lane);
        }

        if (!validMask)
            continue;

        const __m512 vThickness = _mm512_load_ps(thicknesses);
        const __m512 vDensity = _mm512_load_ps(densities);
        const __m512 vResistance = _mm512_load_ps(resistances);
        const __m512 vLosses = _mm512_mul_ps(_mm512_fmadd_ps(vThickness, vDensity, _mm512_setzero_ps()),
                                             _mm512_mul_ps(vResistance, vResistanceScale));
        _mm512_store_ps(losses, vLosses);

        for (size_t lane = 0; lane < batchCount; ++lane) {
            if ((validMask & (1u << lane)) == 0)
                continue;

            const float layerLoss = Material_Density_Energy_Loss(remainingEnergy, densities[lane], thicknesses[lane], resistances[lane]);
            remainingEnergy -= layerLoss;
            accumulatedThickness += thicknesses[lane];
            ++processedLayers;

            if (!Minimum_Kinetic_Threshold(remainingEnergy, minimumThreshold)) {
                outResult.remainingEnergy = (std::max)(remainingEnergy, 0.0f);
                outResult.accumulatedThickness = accumulatedThickness;
                outResult.processedLayers = processedLayers;
                outResult.reachedTarget = false;
                return false;
            }
        }
    }

    outResult.remainingEnergy = (std::max)(remainingEnergy, 0.0f);
    outResult.accumulatedThickness = accumulatedThickness;
    outResult.processedLayers = processedLayers;
    outResult.reachedTarget = (remainingEnergy > minimumThreshold) && (accumulatedThickness <= ray.maxDistance);
    return outResult.reachedTarget;
}

bool CPhysicsCriticalPhases::Phase2::Volumetric_Penetration_Solver_TraceIO(const PenetrationRay& ray,
                                                                            float initialEnergy,
                                                                            float minimumThreshold,
                                                                            uint32_t maxImpacts,
                                                                            PenetrationResult& outResult)
{
    outResult = {};
    if (maxImpacts == 0 || initialEnergy <= 0.0f || ray.maxDistance <= 0.0f)
        return false;

    CL_Trace* pTrace = GetCL_Trace();
    if (!pTrace)
        return false;

    Vector3 dir = ray.direction;
    const float dirLenSq = dir.m_x * dir.m_x + dir.m_y * dir.m_y + dir.m_z * dir.m_z;
    if (dirLenSq <= 1.0e-8f)
        return false;
    const float invLen = 1.0f / std::sqrt(dirLenSq);
    dir = dir * invLen;

    Vector3 currentStart = ray.start;
    float remainingDistance = ray.maxDistance;
    float remainingEnergy = initialEnergy;

    for (uint32_t impact = 0; impact < maxImpacts && remainingDistance > 0.0f; ++impact) {
        const Vector3 traceEnd = currentStart + (dir * remainingDistance);
        TracePhysicsSample sample{};
        if (!pTrace->TracePhysicsSegment(currentStart, traceEnd, sample)) {
            outResult.reachedTarget = true;
            break;
        }

        if (!sample.didHit || sample.fraction >= 1.0f) {
            outResult.reachedTarget = true;
            break;
        }

        const float traveled = (std::max)(0.0f, remainingDistance * sample.fraction);
        const float layerLoss = Material_Density_Energy_Loss(
            remainingEnergy,
            sample.density,
            traveled,
            sample.penetrationResistance
        );

        remainingEnergy -= layerLoss;
        outResult.accumulatedThickness += traveled;
        ++outResult.processedLayers;

        if (!Minimum_Kinetic_Threshold(remainingEnergy, minimumThreshold)) {
            outResult.remainingEnergy = (std::max)(remainingEnergy, 0.0f);
            outResult.reachedTarget = false;
            return false;
        }

        currentStart = sample.hitPosition + (dir * 0.5f);
        remainingDistance -= (traveled + 0.5f);
    }

    outResult.remainingEnergy = (std::max)(remainingEnergy, 0.0f);
    return outResult.reachedTarget;
}

float CPhysicsCriticalPhases::Phase2::Material_Density_Energy_Loss(float energyIn, float density, float thickness, float resistance)
{
    const float normalizedDensity = (std::max)(density, 0.0f);
    const float normalizedThickness = (std::max)(thickness, 0.0f);
    const float normalizedResistance = (std::max)(resistance, 0.0f);
    const float attenuation = normalizedDensity * normalizedThickness * normalizedResistance;
    const float cappedAttenuation = (std::min)(attenuation, 0.98f);
    return energyIn * cappedAttenuation;
}

bool CPhysicsCriticalPhases::Phase2::Minimum_Kinetic_Threshold(float remainingEnergy, float minimumThreshold)
{
    return remainingEnergy >= minimumThreshold;
}

bool CPhysicsCriticalPhases::Phase2::Non_Linear_Phase_Bypass(uint32_t layerFlags)
{
    return (layerFlags & kLayerFlagDynamic) != 0;
}

float CPhysicsCriticalPhases::Phase2::Bisection_Thickness_Check(float entryDistance, float exitDistance, uint32_t maxIterations)
{
    if (exitDistance <= entryDistance)
        return 0.0f;

    float low = entryDistance;
    float high = exitDistance;
    for (uint32_t i = 0; i < maxIterations; ++i) {
        const float mid = 0.5f * (low + high);
        const bool leftHasBoundary = (mid - entryDistance) > (exitDistance - mid);
        if (leftHasBoundary)
            high = mid;
        else
            low = mid;
    }

    return (std::max)(0.0f, exitDistance - entryDistance);
}

void CPhysicsCriticalPhases::Phase4::Asynchronous_Mesh_Decoupling(SensorMatrixState& ioState, float decouplingStrength, float dt)
{
    const float strength = (std::clamp)(decouplingStrength, 0.0f, 1.0f);
    const float blend = (std::clamp)(dt * 60.0f * strength, 0.0f, 1.0f);

    const Vector3 deltaAngles = ioState.collisionAngles - ioState.renderAngles;
    ioState.renderAngles = ioState.renderAngles + (deltaAngles * blend);
    ioState.renderOrigin = ioState.renderOrigin + ((ioState.collisionOrigin - ioState.renderOrigin) * blend);
}

void CPhysicsCriticalPhases::Phase4::Zenith_Nadir_Gimbal_Lock(SensorMatrixState& ioState, bool lockToZenith)
{
    ioState.collisionAngles.m_x = lockToZenith ? kPitchClamp : -kPitchClamp;
    ioState.renderAngles.m_x = ioState.collisionAngles.m_x;
}

void CPhysicsCriticalPhases::Phase4::Stochastic_Yaw_Dispersion(SensorMatrixState& ioState, uint32_t& rngState, float jitterAmplitude)
{
    rngState ^= (rngState << 13);
    rngState ^= (rngState >> 17);
    rngState ^= (rngState << 5);

    const float random01 = (rngState & 0x00FFFFFF) / static_cast<float>(0x01000000);
    const float signedNoise = (random01 * 2.0f) - 1.0f;
    ioState.renderAngles.m_y += signedNoise * jitterAmplitude;

    while (ioState.renderAngles.m_y > kYawWrap) ioState.renderAngles.m_y -= 360.0f;
    while (ioState.renderAngles.m_y < -kYawWrap) ioState.renderAngles.m_y += 360.0f;
}

void CPhysicsCriticalPhases::Phase4::Lower_Body_Kinematic_Break(SensorMatrixState& ioState, KinematicOscillationState& ioKinematics, float dt, float breakRateDeg)
{
    ioKinematics.lbyAccumulator += breakRateDeg * dt;
    if (ioKinematics.lbyAccumulator > 360.0f)
        ioKinematics.lbyAccumulator -= 360.0f;

    const float phase = ioKinematics.lbyAccumulator * kDegToRad;
    const float torsion = std::sin(phase) * breakRateDeg;

    ioState.renderAngles.m_z += torsion;
    ioState.collisionAngles.m_z -= torsion * 0.5f;

    if (ioState.renderAngles.m_z > 180.0f) ioState.renderAngles.m_z -= 360.0f;
    if (ioState.renderAngles.m_z < -180.0f) ioState.renderAngles.m_z += 360.0f;
    if (ioState.collisionAngles.m_z > 180.0f) ioState.collisionAngles.m_z -= 360.0f;
    if (ioState.collisionAngles.m_z < -180.0f) ioState.collisionAngles.m_z += 360.0f;
}

void CPhysicsCriticalPhases::Phase4::High_Frequency_Z_Axis_Oscillation(SensorMatrixState& ioState,
                                                                        KinematicOscillationState& ioKinematics,
                                                                        float amplitude,
                                                                        float frequencyHz,
                                                                        float dt)
{
    const float clampedAmp = (std::max)(0.0f, amplitude);
    const float clampedFreq = (std::max)(0.0f, frequencyHz);
    ioKinematics.zOscillationPhase += kTwoPi * clampedFreq * dt;
    if (ioKinematics.zOscillationPhase > kTwoPi)
        ioKinematics.zOscillationPhase = std::fmod(ioKinematics.zOscillationPhase, kTwoPi);

    const float zOffset = std::sin(ioKinematics.zOscillationPhase) * clampedAmp;
    ioState.renderOrigin.m_z = ioState.collisionOrigin.m_z + zOffset;
    ioKinematics.lastTickDt = dt;
}

void CPhysicsCriticalPhases::Phase4::Micro_Movement_State_Preservation(SensorMatrixState& ioState, float epsilonStep)
{
    const float epsilon = (std::clamp)(epsilonStep, 0.0f, 0.01f);
    ioState.collisionOrigin.m_x += epsilon;
    ioState.collisionOrigin.m_y -= epsilon;
    ioState.renderOrigin.m_x -= epsilon;
    ioState.renderOrigin.m_y += epsilon;
}

void CPhysicsCriticalPhases::Phase4::Inertial_Damping_Override(Vector3& ioVelocity)
{
    const __m128 velocity = _mm_set_ps(0.0f, ioVelocity.m_z, ioVelocity.m_y, ioVelocity.m_x);
    const __m128 damping = _mm_set1_ps(0.0f);
    const __m128 damped = _mm_mul_ps(velocity, damping);

    alignas(16) float out[4];
    _mm_store_ps(out, damped);
    ioVelocity.m_x = out[0];
    ioVelocity.m_y = out[1];
    ioVelocity.m_z = out[2];
}

CPhysicsCriticalPhases::DecryptionState CPhysicsCriticalPhases::Phase6::Heuristic_Angle_Decryption(const ExternalEntitySoA& entities,
                                                                                                    size_t entityIndex,
                                                                                                    float referenceYaw)
{
    DecryptionState out{};
    if (entityIndex >= entities.count || !entities.baseYaw)
        return out;

    const float baseYaw = entities.baseYaw[entityIndex];
    const __m128 candidateOffsets = _mm_set_ps(0.0f, 60.0f, 0.0f, -60.0f);
    const __m128 base = _mm_set1_ps(baseYaw);
    const __m128 ref = _mm_set1_ps(referenceYaw);
    const __m128 candidates = _mm_add_ps(base, candidateOffsets);
    const __m128 delta = _mm_sub_ps(candidates, ref);

    alignas(16) float candidateVals[4];
    alignas(16) float deltaVals[4];
    _mm_store_ps(candidateVals, candidates);
    _mm_store_ps(deltaVals, delta);

    float bestScore = std::numeric_limits<float>::max();
    float bestYaw = baseYaw;
    constexpr size_t kEvalCount = 3;
    for (size_t i = 0; i < kEvalCount; ++i) {
        const float wrapped = WrapYaw(candidateVals[i]);
        const float score = std::fabs(WrapYaw(deltaVals[i]));
        if (score < bestScore) {
            bestScore = score;
            bestYaw = wrapped;
        }
    }

    const float leanLayer = entities.lean ? entities.lean[entityIndex] : 0.0f;
    const float moveYawLayer = entities.moveYaw ? entities.moveYaw[entityIndex] : 0.0f;
    out.bruteYaw = bestYaw;
    out.overlayYaw = Animation_Overlay_Synchronization(baseYaw, leanLayer, moveYawLayer);
    out.correctedYaw = Asymmetrical_Desync_Correction(out.bruteYaw, out.overlayYaw, std::fabs(leanLayer));
    out.desyncDelta = WrapYaw(out.correctedYaw - baseYaw);
    return out;
}

float CPhysicsCriticalPhases::Phase6::Animation_Overlay_Synchronization(float baseYaw, float leanLayer, float moveYawLayer)
{
    const float leanInfluence = (std::clamp)(leanLayer * 0.35f, -35.0f, 35.0f);
    const float moveInfluence = (std::clamp)(moveYawLayer * 0.65f, -65.0f, 65.0f);
    return WrapYaw(baseYaw + leanInfluence + moveInfluence);
}

float CPhysicsCriticalPhases::Phase6::Asymmetrical_Desync_Correction(float bruteYaw, float overlayYaw, float leanLayerWeight)
{
    const float blend = (std::clamp)(leanLayerWeight, 0.0f, 1.0f);
    const float overlayDelta = WrapYaw(overlayYaw - bruteYaw);
    return WrapYaw(bruteYaw + (overlayDelta * blend));
}

void CPhysicsCriticalPhases::Phase6::Predictive_State_Extrapolation(ExternalEntitySoA& entities,
                                                                     float missingDeltaTime,
                                                                     float gravityAcceleration)
{
    if (!entities.posX || !entities.posY || !entities.posZ || !entities.velX || !entities.velY || !entities.velZ || entities.count == 0)
        return;

    const float dt = (std::max)(0.0f, missingDeltaTime);
    const float dt2 = dt * dt;
    const __m512 vDt = _mm512_set1_ps(dt);
    const __m512 vHalfDt2 = _mm512_set1_ps(0.5f * dt2);
    const __m512 vGravity = _mm512_set1_ps(gravityAcceleration);

    for (size_t i = 0; i < entities.count; i += 16) {
        const size_t remaining = entities.count - i;
        const __mmask16 laneMask = static_cast<__mmask16>((remaining >= 16) ? 0xFFFFu : ((1u << remaining) - 1u));

        __m512 posX = _mm512_maskz_loadu_ps(laneMask, &entities.posX[i]);
        __m512 posY = _mm512_maskz_loadu_ps(laneMask, &entities.posY[i]);
        __m512 posZ = _mm512_maskz_loadu_ps(laneMask, &entities.posZ[i]);
        __m512 velX = _mm512_maskz_loadu_ps(laneMask, &entities.velX[i]);
        __m512 velY = _mm512_maskz_loadu_ps(laneMask, &entities.velY[i]);
        __m512 velZ = _mm512_maskz_loadu_ps(laneMask, &entities.velZ[i]);

        posX = _mm512_fmadd_ps(velX, vDt, posX);
        posY = _mm512_fmadd_ps(velY, vDt, posY);
        posZ = _mm512_fmadd_ps(velZ, vDt, _mm512_fmadd_ps(vGravity, vHalfDt2, posZ));
        velZ = _mm512_fmadd_ps(vGravity, vDt, velZ);

        _mm512_mask_storeu_ps(&entities.posX[i], laneMask, posX);
        _mm512_mask_storeu_ps(&entities.posY[i], laneMask, posY);
        _mm512_mask_storeu_ps(&entities.posZ[i], laneMask, posZ);
        _mm512_mask_storeu_ps(&entities.velZ[i], laneMask, velZ);
    }
}
