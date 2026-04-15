#pragma once

#include <Common/Common.hpp>

#include <CS2/SDK/Math/Vector3.hpp>

#include <functional>
#include <vector>

class CBallisticsSystem final
{
public:
	enum class EMaterial : uint8_t
	{
		AIR = 0,
		WOOD,
		CONCRETE,
		METAL,
		GLASS
	};

	struct MaterialPenetration_t
	{
		EMaterial Material = EMaterial::AIR;
		float Resistance = 0.f;
	};

	struct RaycastHit_t
	{
		bool bHit = false;
		Vector3 ImpactPoint{};
		Vector3 ExitPoint{};
		EMaterial Material = EMaterial::AIR;
		float Thickness = 0.f;
	};

	using RaycastFn_t = std::function<RaycastHit_t( const Vector3& Start , const Vector3& End )>;

	struct Settings_t
	{
		float BaseDamage = 100.f;
		float MinDamageThreshold = 20.f;
		int MaxPenetrations = 4;
	};

	struct TrajectoryPoint_t
	{
		Vector3 Start{};
		Vector3 End{};
		float DamageBefore = 0.f;
		float DamageAfter = 0.f;
		EMaterial Material = EMaterial::AIR;
	};

	struct Result_t
	{
		bool bCanHit = false;
		float FinalDamage = 0.f;
		std::vector<TrajectoryPoint_t> DebugTrajectory{};
	};

public:
	auto SetSettings( const Settings_t& Settings ) -> void;
	auto SetRaycastCallback( RaycastFn_t Callback ) -> void;
	auto SetMaterialTable( const std::vector<MaterialPenetration_t>& Table ) -> void;

	auto Evaluate( const Vector3& Start , const Vector3& End ) const -> Result_t;
	auto GetMaterialResistance( EMaterial Material ) const -> float;

private:
	Settings_t m_Settings{};
	RaycastFn_t m_RaycastCallback{};
	std::vector<MaterialPenetration_t> m_MaterialTable{};
};

auto GetBallisticsSystem() -> CBallisticsSystem*;
