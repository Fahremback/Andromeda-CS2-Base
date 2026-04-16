#pragma once

#include <Common/Common.hpp>
#include <vector>

#include <CS2/SDK/Math/Vector3.hpp>

class C_CSPlayerPawn;

extern const std::vector<const char*> g_AllTraceVisibleCheckBones;

class CL_Bones final
{
public:
	auto GetBonePositionByName( C_CSPlayerPawn* pC_CSPlayerPawn , const char* szBoneName ) -> Vector3;
	auto GetBonePositionsByName( C_CSPlayerPawn* pC_CSPlayerPawn , const char* const* BoneNames , size_t BoneCount , Vector3* BonePositions ) -> bool;
	auto GetBonePositionsByName( C_CSPlayerPawn* pC_CSPlayerPawn , const std::vector<const char*>& BoneNames , std::vector<Vector3>& BonePositions ) -> bool;
};

auto GetCL_Bones() -> CL_Bones*;
