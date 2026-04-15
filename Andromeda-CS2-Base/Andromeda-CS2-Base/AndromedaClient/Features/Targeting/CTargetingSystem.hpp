#pragma once

#include <Common/Common.hpp>

#include <CS2/SDK/Math/Vector2.hpp>
#include <CS2/SDK/Math/Vector3.hpp>
#include <CS2/SDK/Types/CHandle.hpp>

#include <GameClient/CEntityCache/CEntityCache.hpp>

#include <cstdint>

class CTargetingSystem final
{
public:
	struct Settings_t
	{
		bool Active = true;
		bool RequireVisibility = true;
		float MaxFovPixels = 240.f;
		float WeightFov = 0.7f;
		float WeightDistance = 0.3f;
		float Smoothing = 6.f;
	};

	struct TargetResult_t
	{
		bool bHasTarget = false;
		CHandle TargetHandle = { INVALID_EHANDLE_INDEX };
		Vector2 TargetScreen{};
		Vector2 RawDelta{};
		Vector2 SmoothedDelta{};
		float Score = 0.f;
	};

	struct StageProfiling_t
	{
		double SnapshotUs = 0.0;
		double ScoreUs = 0.0;
		double TotalUs = 0.0;
		uint32_t CandidateCount = 0;
	};

public:
	auto OnCreateMove() -> void;
	auto OnRenderDebug() -> void;

	auto GetLastResult() const -> const TargetResult_t&;
	auto GetLastProfiling() const -> const StageProfiling_t&;
	auto SetSettings( const Settings_t& Settings ) -> void;

private:
	auto CalculateSmoothedDelta( const Vector2& RawDelta ) const -> Vector2;
	auto ScoreTarget( float FovDistancePixels , float WorldDistance ) const -> float;

private:
	Settings_t m_Settings{};
	TargetResult_t m_LastResult{};
	StageProfiling_t m_LastProfiling{};
};

auto GetTargetingSystem() -> CTargetingSystem*;
