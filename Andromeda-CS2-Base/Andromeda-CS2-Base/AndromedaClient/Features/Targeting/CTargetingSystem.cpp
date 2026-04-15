#include "CTargetingSystem.hpp"

#include <Common/LinearArena.hpp>

#include <AndromedaClient/Fonts/CFontManager.hpp>
#include <AndromedaClient/Render/CRender.hpp>
#include <AndromedaClient/Render/CRenderStackSystem.hpp>
#include <AndromedaClient/Settings/Settings.hpp>

#include <ImGui/imgui.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>

static CTargetingSystem g_TargetingSystem{};

namespace
{
	struct SnapshotSoA_t
	{
		CHandle* pHandles = nullptr;
		Vector2* pCenters = nullptr;
		float* pWorldDistance = nullptr;
		bool* pVisible = nullptr;
		size_t Count = 0;
	};
}

auto CTargetingSystem::ScoreTarget( float FovDistancePixels , float WorldDistance ) const -> float
{
	const float FovNorm = std::clamp( FovDistancePixels / std::max( 1.f , m_Settings.MaxFovPixels ) , 0.f , 1.f );
	const float DistNorm = std::clamp( WorldDistance / 5000.f , 0.f , 1.f );
	const float Score = ( m_Settings.WeightFov * FovNorm ) + ( m_Settings.WeightDistance * DistNorm );
	return Score;
}

auto CTargetingSystem::CalculateSmoothedDelta( const Vector2& RawDelta ) const -> Vector2
{
	const float Smooth = std::max( 1.f , m_Settings.Smoothing );
	return RawDelta / Smooth;
}

auto CTargetingSystem::OnCreateMove() -> void
{
	m_LastResult = {};
	m_LastProfiling = {};
	m_Settings.Active = Settings::Targeting::Active;
	m_Settings.RequireVisibility = Settings::Targeting::RequireVisibility;
	m_Settings.MaxFovPixels = Settings::Targeting::MaxFovPixels;
	m_Settings.WeightFov = Settings::Targeting::WeightFov;
	m_Settings.WeightDistance = Settings::Targeting::WeightDistance;
	m_Settings.Smoothing = Settings::Targeting::Smoothing;

	if ( !m_Settings.Active )
		return;

	auto* pArena = GetLinearArena();
	if ( !pArena )
		return;

	const auto T0 = std::chrono::high_resolution_clock::now();
	const ImVec2 ScreenCenter( ImGui::GetIO().DisplaySize.x * 0.5f , ImGui::GetIO().DisplaySize.y * 0.5f );

	SnapshotSoA_t Snapshot{};
	{
		std::scoped_lock lock( GetEntityCache()->GetLock() );

		const size_t Count = GetEntityCache()->GetCount();
		if ( Count == 0 )
			return;

		Snapshot.pHandles = pArena->AllocateArray<CHandle>( Count );
		Snapshot.pCenters = pArena->AllocateArray<Vector2>( Count );
		Snapshot.pWorldDistance = pArena->AllocateArray<float>( Count );
		Snapshot.pVisible = pArena->AllocateArray<bool>( Count );

		if ( !Snapshot.pHandles || !Snapshot.pCenters || !Snapshot.pWorldDistance || !Snapshot.pVisible )
			return;

		for ( size_t i = 0; i < Count; ++i )
		{
			if ( GetEntityCache()->GetType( i ) != CachedEntity_t::PLAYER_CONTROLLER || !GetEntityCache()->ShouldDraw( i ) )
				continue;

			const auto BBox = GetEntityCache()->GetBBox( i );
			const float CenterX = ( BBox.x + BBox.w ) * 0.5f;
			const float CenterY = ( BBox.y + BBox.h ) * 0.5f;
			const float Dx = ( CenterX - ScreenCenter.x );
			const float Dy = ( CenterY - ScreenCenter.y );

			Snapshot.pHandles[Snapshot.Count] = GetEntityCache()->GetHandle( i );
			Snapshot.pCenters[Snapshot.Count] = { CenterX , CenterY };
			Snapshot.pWorldDistance[Snapshot.Count] = std::sqrt( Dx * Dx + Dy * Dy );
			Snapshot.pVisible[Snapshot.Count] = GetEntityCache()->IsVisible( i );
			++Snapshot.Count;
		}
	}

	const auto T1 = std::chrono::high_resolution_clock::now();

	if ( Snapshot.Count == 0 )
		return;

	float BestScore = std::numeric_limits<float>::max();
	size_t BestIndex = std::numeric_limits<size_t>::max();

#if defined( __AVX2__ )
	for ( size_t i = 0; i < Snapshot.Count; i += 8 )
#else
	for ( size_t i = 0; i < Snapshot.Count; ++i )
#endif
	{
#if defined( __AVX2__ )
		const size_t End = std::min( i + 8 , Snapshot.Count );
		for ( size_t j = i; j < End; ++j )
#else
		const size_t j = i;
#endif
		{
			if ( m_Settings.RequireVisibility && !Snapshot.pVisible[j] )
				continue;

			const float FovDist = Snapshot.pCenters[j].Distance( { ScreenCenter.x , ScreenCenter.y } );
			if ( FovDist > m_Settings.MaxFovPixels )
				continue;

			const float Score = ScoreTarget( FovDist , Snapshot.pWorldDistance[j] );
			if ( Score < BestScore )
			{
				BestScore = Score;
				BestIndex = j;
			}
		}
	}

	const auto T2 = std::chrono::high_resolution_clock::now();

	if ( BestIndex != std::numeric_limits<size_t>::max() )
	{
		m_LastResult.bHasTarget = true;
		m_LastResult.TargetHandle = Snapshot.pHandles[BestIndex];
		m_LastResult.TargetScreen = Snapshot.pCenters[BestIndex];
		m_LastResult.RawDelta = { Snapshot.pCenters[BestIndex].m_x - ScreenCenter.x , Snapshot.pCenters[BestIndex].m_y - ScreenCenter.y };
		m_LastResult.SmoothedDelta = CalculateSmoothedDelta( m_LastResult.RawDelta );
		m_LastResult.Score = BestScore;
	}

	m_LastProfiling.SnapshotUs = static_cast<double>( std::chrono::duration_cast<std::chrono::microseconds>( T1 - T0 ).count() );
	m_LastProfiling.ScoreUs = static_cast<double>( std::chrono::duration_cast<std::chrono::microseconds>( T2 - T1 ).count() );
	m_LastProfiling.TotalUs = static_cast<double>( std::chrono::duration_cast<std::chrono::microseconds>( T2 - T0 ).count() );
	m_LastProfiling.CandidateCount = static_cast<uint32_t>( Snapshot.Count );
}

auto CTargetingSystem::OnRenderDebug() -> void
{
	if ( !m_Settings.Active )
		return;

	const ImVec2 ScreenCenter( ImGui::GetIO().DisplaySize.x * 0.5f , ImGui::GetIO().DisplaySize.y * 0.5f );
	GetRenderStackSystem()->DrawCircle( ScreenCenter , m_Settings.MaxFovPixels , ImColor( 255 , 255 , 0 , 80 ) );

	if ( m_LastResult.bHasTarget )
	{
		GetRenderStackSystem()->DrawLine( ScreenCenter , { m_LastResult.TargetScreen.m_x , m_LastResult.TargetScreen.m_y } , ImColor( 0 , 220 , 0 , 200 ) , 1.5f );
		GetRenderStackSystem()->DrawCircleFilled( { m_LastResult.TargetScreen.m_x , m_LastResult.TargetScreen.m_y } , 3.f , ImColor( 0 , 220 , 0 , 255 ) );
	}

	GetRenderStackSystem()->DrawString( &GetFontManager()->m_VerdanaFont , 8 , 40 , FW1_LEFT , ImColor( 255 , 255 , 255 , 220 ) ,
		"Target candidates=%u | snapshot=%.1fus score=%.1fus total=%.1fus" ,
		m_LastProfiling.CandidateCount , m_LastProfiling.SnapshotUs , m_LastProfiling.ScoreUs , m_LastProfiling.TotalUs );
}

auto CTargetingSystem::GetLastResult() const -> const TargetResult_t&
{
	return m_LastResult;
}

auto CTargetingSystem::GetLastProfiling() const -> const StageProfiling_t&
{
	return m_LastProfiling;
}

auto CTargetingSystem::SetSettings( const Settings_t& Settings ) -> void
{
	m_Settings = Settings;
}

auto GetTargetingSystem() -> CTargetingSystem*
{
	return &g_TargetingSystem;
}
