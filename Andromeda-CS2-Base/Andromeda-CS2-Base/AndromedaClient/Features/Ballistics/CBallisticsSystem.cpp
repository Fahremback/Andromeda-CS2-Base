#include "CBallisticsSystem.hpp"

#include <algorithm>

static CBallisticsSystem g_BallisticsSystem{};

auto CBallisticsSystem::SetSettings( const Settings_t& Settings ) -> void
{
	m_Settings = Settings;
}

auto CBallisticsSystem::SetRaycastCallback( RaycastFn_t Callback ) -> void
{
	m_RaycastCallback = std::move( Callback );
}

auto CBallisticsSystem::SetMaterialTable( const std::vector<MaterialPenetration_t>& Table ) -> void
{
	m_MaterialTable = Table;
}

auto CBallisticsSystem::GetMaterialResistance( EMaterial Material ) const -> float
{
	auto It = std::find_if( m_MaterialTable.begin() , m_MaterialTable.end() , [&]( const MaterialPenetration_t& Item )
	{
		return Item.Material == Material;
	} );

	if ( It == m_MaterialTable.end() )
		return 0.f;

	return std::max( 0.f , It->Resistance );
}

auto CBallisticsSystem::Evaluate( const Vector3& Start , const Vector3& End ) const -> Result_t
{
	Result_t Result{};
	Result.FinalDamage = std::max( 0.f , m_Settings.BaseDamage );

	if ( !m_RaycastCallback )
		return Result;

	Vector3 CurrentStart = Start;

	for ( int Penetration = 0; Penetration <= m_Settings.MaxPenetrations; ++Penetration )
	{
		if ( Result.FinalDamage <= 0.f )
			break;

		auto Hit = m_RaycastCallback( CurrentStart , End );

		if ( !Hit.bHit )
		{
			Result.bCanHit = Result.FinalDamage >= m_Settings.MinDamageThreshold;
			break;
		}

		const float Resistance = GetMaterialResistance( Hit.Material );
		const float Loss = std::max( 0.f , Hit.Thickness ) * Resistance;

		TrajectoryPoint_t Point{};
		Point.Start = CurrentStart;
		Point.End = Hit.ImpactPoint;
		Point.Material = Hit.Material;
		Point.DamageBefore = Result.FinalDamage;
		Point.DamageAfter = std::max( 0.f , Result.FinalDamage - Loss );
		Result.DebugTrajectory.emplace_back( Point );

		Result.FinalDamage = Point.DamageAfter;
		CurrentStart = Hit.ExitPoint;
	}

	Result.bCanHit = ( Result.FinalDamage >= m_Settings.MinDamageThreshold );
	return Result;
}

auto GetBallisticsSystem() -> CBallisticsSystem*
{
	return &g_BallisticsSystem;
}
