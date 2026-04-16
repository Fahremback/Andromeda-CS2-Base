#include "CL_Bones.hpp"

#include <CS2/SDK/Types/CEntityData.hpp>

static CL_Bones g_CL_Bones{};

const std::vector<const char*> g_AllTraceVisibleCheckBones =
{
	"head_0",
	"pelvis",
	"arm_lower_r",
	"arm_lower_l",
	"ankle_r",
	"ankle_l",
};

auto CL_Bones::GetBonePositionByName( C_CSPlayerPawn* pC_CSPlayerPawn , const char* szBoneName ) -> Vector3
{
	Vector3 BonePosition;

	if ( auto* pSkeletonInstance = pC_CSPlayerPawn->m_pGameSceneNode()->GetSkeletonInstance(); pSkeletonInstance )
	{
		pSkeletonInstance->CalcWorldSpaceBones( FLAG_ALL_BONE_FLAGS );

		const auto& Model = pSkeletonInstance->m_modelState().m_hModel();

		if ( !Model.is_valid() )
			return BonePosition;

		const auto BoneIndex = pC_CSPlayerPawn->GetBoneIdByName( szBoneName );

		if ( BoneIndex != -1 && pSkeletonInstance->GetBonePosition( BoneIndex , BonePosition ) )
			return BonePosition;
		else
			DEV_LOG( "[error] GetBoneIdByName: %s\n" , szBoneName );
	}

	return BonePosition;
}

auto CL_Bones::GetBonePositionsByName( C_CSPlayerPawn* pC_CSPlayerPawn , const std::vector<const char*>& BoneNames , std::vector<Vector3>& BonePositions ) -> bool
{
	BonePositions.clear();
	BonePositions.resize( BoneNames.size() );

	return GetBonePositionsByName( pC_CSPlayerPawn , BoneNames.data() , BoneNames.size() , BonePositions.data() );
}

auto CL_Bones::GetBonePositionsByName( C_CSPlayerPawn* pC_CSPlayerPawn , const char* const* BoneNames , size_t BoneCount , Vector3* BonePositions ) -> bool
{
	if ( BoneCount == 0 || !BonePositions )
		return false;

	if ( !pC_CSPlayerPawn )
		return false;

	auto* pSkeletonInstance = pC_CSPlayerPawn->m_pGameSceneNode()->GetSkeletonInstance();
	if ( !pSkeletonInstance )
		return false;

	pSkeletonInstance->CalcWorldSpaceBones( FLAG_ALL_BONE_FLAGS );

	const auto& Model = pSkeletonInstance->m_modelState().m_hModel();
	if ( !Model.is_valid() )
		return false;

	auto HasAnyValidBone = false;

	for ( size_t i = 0; i < BoneCount; ++i )
	{
		const auto BoneIndex = pC_CSPlayerPawn->GetBoneIdByName( BoneNames[i] );
		if ( BoneIndex != -1 && pSkeletonInstance->GetBonePosition( BoneIndex , BonePositions[i] ) )
			HasAnyValidBone = true;
		else
			BonePositions[i] = {};
	}

	return HasAnyValidBone;
}

auto GetCL_Bones() -> CL_Bones*
{
	return &g_CL_Bones;
}
