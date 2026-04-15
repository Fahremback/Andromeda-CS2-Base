#pragma once

#include <Common/Common.hpp>

#include <array>
#include <atomic>
#include <cstdint>
#include <vector>

class CStageProfiler final
{
public:
	enum class EStage : uint8_t
	{
		INPUT = 0,
		SIMULATION,
		RENDER_SUBMIT,
		COUNT
	};

	struct StageStats_t
	{
		double P50Us = 0.0;
		double P95Us = 0.0;
		double P99Us = 0.0;
		double P999Us = 0.0;
		double MaxUs = 0.0;
	};

public:
	auto PushSample( EStage Stage , double DurationUs ) -> void;
	auto Snapshot( EStage Stage ) const -> StageStats_t;
	auto SetBudgetUs( EStage Stage , double BudgetUs ) -> void;
	auto IsWithinBudget( EStage Stage , double DurationUs ) const -> bool;

private:
	struct alignas( 64 ) StageBuffer_t
	{
		static constexpr size_t MAX_SAMPLES = 2048;
		std::array<double , MAX_SAMPLES> Samples{};
		std::atomic_size_t WriteIndex = 0;
		std::atomic_size_t Count = 0;
	};

private:
	std::array<StageBuffer_t , static_cast<size_t>( EStage::COUNT )> m_StageBuffers{};
	std::array<double , static_cast<size_t>( EStage::COUNT )> m_BudgetsUs{};
};
