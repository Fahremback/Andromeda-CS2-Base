#pragma once

#include <Common/Common.hpp>
#include <CS2/SDK/Update/GameTrace.hpp>

#include "CL_Bones.hpp"

class QAngle;
class Vector3;

class CCSGOInput;
class C_BaseEntity;

struct TracePhysicsSample
{
	float fraction = 1.0f;
	float density = 1.0f;
	float penetrationResistance = 1.0f;
	uint32_t surfaceFlags = 0;
	bool didHit = false;
	Vector3 hitPosition{ 0, 0, 0 };
	Vector3 normal{ 0, 0, 0 };
};

class CL_Trace final
{
public:
	auto TraceToBoneEntity( CCSGOInput* pInput , const QAngle* AngleCorrection , QAngle* ViewAngleCorrection ) -> std::pair<uint64_t , C_BaseEntity*>;
	auto TraceToEntityEndPos( const Vector3* vEnd ) -> C_BaseEntity*;
	auto TracePhysicsSegment( const Vector3& vStart , const Vector3& vEnd , TracePhysicsSample& outSample ) -> bool;
};

auto GetCL_Trace() -> CL_Trace*;
