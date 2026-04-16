#include "CVisual.hpp"
#include <array>

#include <CS2/SDK/SDK.hpp>
#include <CS2/SDK/Interface/IEngineToClient.hpp>

#include <AndromedaClient/Render/CRender.hpp>
#include <AndromedaClient/Render/CRenderStackSystem.hpp>
#include <AndromedaClient/Settings/Settings.hpp>

#include <GameClient/CEntityCache/CEntityCache.hpp>
#include <GameClient/CL_Players.hpp>
#include <GameClient/CL_VisibleCheck.hpp>
#include <GameClient/CL_Bones.hpp>
#include <CS2/SDK/Math/Math.hpp>
#include <CS2/SDK/Types/CEntityData.hpp>
#include <CS2/SDK/Types/Color.hpp>

static CVisual g_CVisual{};

auto CVisual::OnRender() -> void
{
	if ( !Active_Setting )
		return;

	// Calculate bounding boxes on the render thread with throttling
	CalculateBoundingBoxes();

	const auto& CachedVec = GetEntityCache()->GetCachedEntity();

	std::scoped_lock Lock( GetEntityCache()->GetLock() );

	for ( const auto& CachedEntity : *CachedVec )
	{
		auto pEntity = CachedEntity.m_Handle.Get();

		if ( !pEntity )
			continue;

		// FIX: Validate entity handle BEFORE accessing pEntityIdentity to prevent
		// use-after-free during match transitions
		auto hEntity = pEntity->pEntityIdentity()->Handle();

		if ( hEntity != CachedEntity.m_Handle )
			continue;

		switch ( CachedEntity.m_Type )
		{
			case CachedEntity_t::PLAYER_CONTROLLER:
			{
				auto* pCCSPlayerController = reinterpret_cast<CCSPlayerController*>( pEntity );

				if ( CachedEntity.m_bDraw )
					OnRenderPlayerEsp( pCCSPlayerController , CachedEntity.m_Bbox , CachedEntity.m_bVisible );
			}
			break;
		}
	}
}

auto CVisual::OnRenderPlayerEsp( CCSPlayerController* pCCSPlayerController , const Rect_t& bBox , const bool bVisible ) -> void
{
	if ( !pCCSPlayerController->m_bPawnIsAlive() )
		return;

	auto* pC_CSPlayerPawn = pCCSPlayerController->m_hPawn().Get<C_CSPlayerPawn>();

	if ( !pC_CSPlayerPawn || !pC_CSPlayerPawn->IsPlayerPawn() )
		return;

	auto IsEnemy = false;

	if ( auto* pLocalPlayerController = GetCL_Players()->GetLocalPlayerController(); pLocalPlayerController )
		IsEnemy = pCCSPlayerController->m_iTeamNum() != pLocalPlayerController->m_iTeamNum();

	ImVec2 min = { bBox.x, bBox.y };
	ImVec2 max = { bBox.w, bBox.h };

	min.x = std::floorf( min.x );
	min.y = std::floorf( min.y );

	max.x = std::ceilf( max.x );
	max.y = std::ceilf( max.y );

	auto Draw = false;

	if ( Team_Setting && !IsEnemy )
		Draw = true;

	if ( Enemy_Setting && IsEnemy )
		Draw = true;

	if ( OnlyVisible_Setting )
	{
		if ( bVisible )
			Draw = true;
		else
			Draw = false;
	}

	if ( Draw )
	{
		if ( PlayerBox_Setting )
		{
			auto PlayerColor = ImColor( 255 , 255 , 255 );

			if ( pCCSPlayerController->m_iTeamNum() == TEAM_TT )
			{
				PlayerColor = PlayerBoxTT_Setting;

				if ( bVisible )
					PlayerColor = PlayerBoxTT_Visible_Setting;
			}
			else if ( pCCSPlayerController->m_iTeamNum() == TEAM_CT )
			{
				PlayerColor = PlayerBoxCT_Setting;

				if ( bVisible )
					PlayerColor = PlayerBoxCT_Visible_Setting;
			}

			if ( PlayerBoxType_Setting == EVisualBoxType_t::BOX )
				GetRenderStackSystem()->DrawBox( min , max , PlayerColor );
			else if ( PlayerBoxType_Setting == EVisualBoxType_t::OUTLINE_BOX )
				GetRenderStackSystem()->DrawOutlineBox( min , max , PlayerColor );
			else if ( PlayerBoxType_Setting == EVisualBoxType_t::COAL_BOX )
				GetRenderStackSystem()->DrawCoalBox( min , max , PlayerColor );
			else if ( PlayerBoxType_Setting == EVisualBoxType_t::OUTLINE_COAL_BOX )
				GetRenderStackSystem()->DrawOutlineCoalBox( min , max , PlayerColor );
		}

		if ( BoneESP_Setting )
		{
			auto ShouldDrawBone = false;

			if ( BoneESPTeam_Setting && !IsEnemy )
				ShouldDrawBone = true;

			if ( BoneESPEnemy_Setting && IsEnemy )
				ShouldDrawBone = true;

			if ( OnlyVisible_Setting && !bVisible )
				ShouldDrawBone = false;

			if ( ShouldDrawBone )
			{
				DrawBoneESP( pC_CSPlayerPawn , bVisible , pCCSPlayerController->m_iTeamNum() );
			}
		}
	}
}

auto CVisual::ApplyGlow( C_CSPlayerPawn* pC_CSPlayerPawn , const bool bVisible , const int iTeamNum ) -> void
{
	if ( !pC_CSPlayerPawn || !pC_CSPlayerPawn->IsPlayerPawn() )
		return;

	auto IsEnemy = false;
	C_CSPlayerPawn* pLocalPawn = nullptr;

	if ( auto* pLocalPlayerController = GetCL_Players()->GetLocalPlayerController(); pLocalPlayerController )
	{
		pLocalPawn = pLocalPlayerController->m_hPawn().Get<C_CSPlayerPawn>();
		if ( pLocalPawn )
		{
			IsEnemy = iTeamNum != pLocalPawn->m_iTeamNum();

			// Local player'dan glow'u kaldır
			if ( pC_CSPlayerPawn == pLocalPawn )
			{
				auto& Glow = pC_CSPlayerPawn->m_Glow();
				Glow.m_bGlowing() = false;
				return;
			}
		}
	}

	auto DrawGlow = false;

	if ( GlowTeam_Setting && !IsEnemy )
		DrawGlow = true;

	if ( GlowEnemy_Setting && IsEnemy )
		DrawGlow = true;

	if ( OnlyVisible_Setting && !bVisible )
		DrawGlow = false;

	if ( !DrawGlow )
	{
		// Glow'u kapat
		auto& Glow = pC_CSPlayerPawn->m_Glow();
		Glow.m_bGlowing() = false;
		return;
	}

	auto GlowColor = Color( 255 , 255 , 255 , 255 );

	if ( iTeamNum == TEAM_TT )
	{
		const auto& ColorVec = GlowTT_Setting;
		if ( bVisible )
		{
			const auto& VisibleColorVec = GlowTT_Visible_Setting;
			GlowColor = Color( 
				static_cast<BYTE>( VisibleColorVec.x * 255.f ) , 
				static_cast<BYTE>( VisibleColorVec.y * 255.f ) , 
				static_cast<BYTE>( VisibleColorVec.z * 255.f ) , 
				static_cast<BYTE>( VisibleColorVec.w * 255.f ) 
			);
		}
		else
		{
			GlowColor = Color( 
				static_cast<BYTE>( ColorVec.x * 255.f ) , 
				static_cast<BYTE>( ColorVec.y * 255.f ) , 
				static_cast<BYTE>( ColorVec.z * 255.f ) , 
				static_cast<BYTE>( ColorVec.w * 255.f ) 
			);
		}
	}
	else if ( iTeamNum == TEAM_CT )
	{
		const auto& ColorVec = GlowCT_Setting;
		if ( bVisible )
		{
			const auto& VisibleColorVec = GlowCT_Visible_Setting;
			GlowColor = Color( 
				static_cast<BYTE>( VisibleColorVec.x * 255.f ) , 
				static_cast<BYTE>( VisibleColorVec.y * 255.f ) , 
				static_cast<BYTE>( VisibleColorVec.z * 255.f ) , 
				static_cast<BYTE>( VisibleColorVec.w * 255.f ) 
			);
		}
		else
		{
			GlowColor = Color( 
				static_cast<BYTE>( ColorVec.x * 255.f ) , 
				static_cast<BYTE>( ColorVec.y * 255.f ) , 
				static_cast<BYTE>( ColorVec.z * 255.f ) , 
				static_cast<BYTE>( ColorVec.w * 255.f ) 
			);
		}
	}

	// Glow'u uygula
	auto& Glow = pC_CSPlayerPawn->m_Glow();
	Glow.m_bGlowing() = true;
	Glow.m_glowColorOverride() = GlowColor;
}

auto CVisual::OnClientOutput() -> void
{
	OnRender();
}

auto CVisual::OnCreateMove() -> void
{
	// FIX: Removed heavy entity iteration (visibility traces + glow) from CreateMove.
	// These operations are now performed in OnRender() on the present thread only.
	// CreateMove is a hot path - running physics traces here blocks the game thread.
	// Visibility and glow are visual-only features; they don't need to run at tick rate.
}

auto CVisual::CalculateBoundingBoxes() -> void
{
	if ( !SDK::Interfaces::EngineToClient()->IsInGame() )
		return;

	// FIX: Throttle bounding box updates to avoid heavy iteration every view matrix call
	m_bboxUpdateCounter++;
	if (m_bboxUpdateCounter % BBOX_UPDATE_INTERVAL != 0)
		return;

	const auto& CachedVec = GetEntityCache()->GetCachedEntity();

	std::scoped_lock Lock( GetEntityCache()->GetLock() );

	for ( auto& it : *CachedVec )
	{
		auto pEntity = it.m_Handle.Get();

		if ( !pEntity )
			continue;

		// FIX: Validate entity handle BEFORE accessing pEntityIdentity to prevent
		// use-after-free on stale pointers during match transitions
		auto hEntity = pEntity->pEntityIdentity()->Handle();

		if ( hEntity != it.m_Handle )
			continue;

		switch ( it.m_Type )
		{
			case CachedEntity_t::PLAYER_CONTROLLER:
			{
				auto pPlayerController = reinterpret_cast<CCSPlayerController*>( pEntity );
				auto pPlayerPawn = pPlayerController->m_hPawn().Get<C_CSPlayerPawn>();

				if ( pPlayerPawn && pPlayerPawn->IsPlayerPawn() && pPlayerController->m_bPawnIsAlive() )
					it.m_bDraw = pPlayerPawn->GetBoundingBox( it.m_Bbox );
			}
			break;
		}
	}
}

auto CVisual::DrawBoneESP( C_CSPlayerPawn* pC_CSPlayerPawn , const bool bVisible , const int iTeamNum ) -> void
{
	if ( !pC_CSPlayerPawn || !pC_CSPlayerPawn->IsPlayerPawn() )
		return;

	auto BoneColor = ImColor( 255 , 255 , 255 );

	if ( iTeamNum == TEAM_TT )
	{
		BoneColor = BoneESPTT_Setting;

		if ( bVisible )
			BoneColor = BoneESPTT_Visible_Setting;
	}
	else if ( iTeamNum == TEAM_CT )
	{
		BoneColor = BoneESPCT_Setting;

		if ( bVisible )
			BoneColor = BoneESPCT_Visible_Setting;
	}

	// Bone connections structure for CS2 skeleton
	struct BoneConnection_t
	{
		size_t nFrom;
		size_t nTo;
	};

	enum EBoneIndex_t : size_t
	{
		BONE_HEAD = 0,
		BONE_NECK,
		BONE_SPINE_1,
		BONE_SPINE_2,
		BONE_SPINE_3,
		BONE_PELVIS,
		BONE_CLAVICLE_L,
		BONE_ARM_UPPER_L,
		BONE_ARM_LOWER_L,
		BONE_HAND_L,
		BONE_CLAVICLE_R,
		BONE_ARM_UPPER_R,
		BONE_ARM_LOWER_R,
		BONE_HAND_R,
		BONE_LEG_UPPER_L,
		BONE_LEG_LOWER_L,
		BONE_ANKLE_L,
		BONE_LEG_UPPER_R,
		BONE_LEG_LOWER_R,
		BONE_ANKLE_R,
		BONE_COUNT
	};

	static const std::array<const char* , BONE_COUNT> RequiredBones =
	{
		"head_0", "neck_0", "spine_1", "spine_2", "spine_3", "pelvis",
		"clavicle_l", "arm_upper_l", "arm_lower_l", "hand_l",
		"clavicle_r", "arm_upper_r", "arm_lower_r", "hand_r",
		"leg_upper_l", "leg_lower_l", "ankle_l",
		"leg_upper_r", "leg_lower_r", "ankle_r"
	};

	static const std::array<BoneConnection_t , 19> BoneConnections = {{
		// Head to spine chain
		{ BONE_HEAD , BONE_NECK },
		{ BONE_NECK , BONE_SPINE_1 },
		{ BONE_SPINE_1 , BONE_SPINE_2 },
		{ BONE_SPINE_2 , BONE_SPINE_3 },
		{ BONE_SPINE_3 , BONE_PELVIS },

		// Left arm
		{ BONE_NECK , BONE_CLAVICLE_L },
		{ BONE_CLAVICLE_L , BONE_ARM_UPPER_L },
		{ BONE_ARM_UPPER_L , BONE_ARM_LOWER_L },
		{ BONE_ARM_LOWER_L , BONE_HAND_L },

		// Right arm
		{ BONE_NECK , BONE_CLAVICLE_R },
		{ BONE_CLAVICLE_R , BONE_ARM_UPPER_R },
		{ BONE_ARM_UPPER_R , BONE_ARM_LOWER_R },
		{ BONE_ARM_LOWER_R , BONE_HAND_R },

		// Left leg
		{ BONE_PELVIS , BONE_LEG_UPPER_L },
		{ BONE_LEG_UPPER_L , BONE_LEG_LOWER_L },
		{ BONE_LEG_LOWER_L , BONE_ANKLE_L },

		// Right leg
		{ BONE_PELVIS , BONE_LEG_UPPER_R },
		{ BONE_LEG_UPPER_R , BONE_LEG_LOWER_R },
		{ BONE_LEG_LOWER_R , BONE_ANKLE_R } 
	}};

	std::array<Vector3 , BONE_COUNT> BonePositions{};
	if ( !GetCL_Bones()->GetBonePositionsByName( pC_CSPlayerPawn , RequiredBones.data() , RequiredBones.size() , BonePositions.data() ) )
		return;

	for ( const auto& Connection : BoneConnections )
	{
		const Vector3& FromBone = BonePositions[Connection.nFrom];
		const Vector3& ToBone = BonePositions[Connection.nTo];

		if ( FromBone.IsZero() || ToBone.IsZero() )
			continue;

		ImVec2 FromScreen , ToScreen;

		if ( Math::WorldToScreen( FromBone , FromScreen ) && Math::WorldToScreen( ToBone , ToScreen ) )
		{
			GetRenderStackSystem()->DrawLine( FromScreen , ToScreen , BoneColor , 1.5f );
		}
	}
}

auto GetVisual() -> CVisual*
{
	return &g_CVisual;
}
