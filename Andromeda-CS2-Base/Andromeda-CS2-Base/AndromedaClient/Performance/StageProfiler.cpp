#include "StageProfiler.hpp"

#include <algorithm>

namespace
{
	template<typename T>
	auto Percentile( std::vector<T>& Values , double Pct ) -> T
	{
		if ( Values.empty() )
			return T{};

		const size_t Idx = static_cast<size_t>( std::clamp( Pct , 0.0 , 1.0 ) * static_cast<double>( Values.size() - 1 ) );
		std::nth_element( Values.begin() , Values.begin() + Idx , Values.end() );
		return Values[Idx];
	}
}

auto CStageProfiler::PushSample( EStage Stage , double DurationUs ) -> void
{
	auto& Buffer = m_StageBuffers[static_cast<size_t>( Stage )];
	const size_t Index = Buffer.WriteIndex.fetch_add( 1 , std::memory_order_relaxed ) % StageBuffer_t::MAX_SAMPLES;
	Buffer.Samples[Index] = DurationUs;

	const size_t Prev = Buffer.Count.load( std::memory_order_relaxed );
	if ( Prev < StageBuffer_t::MAX_SAMPLES )
		Buffer.Count.store( Prev + 1 , std::memory_order_relaxed );
}

auto CStageProfiler::Snapshot( EStage Stage ) const -> StageStats_t
{
	StageStats_t Out{};
	const auto& Buffer = m_StageBuffers[static_cast<size_t>( Stage )];
	const size_t Count = Buffer.Count.load( std::memory_order_relaxed );

	if ( Count == 0 )
		return Out;

	std::vector<double> Copy;
	Copy.reserve( Count );
	for ( size_t i = 0; i < Count; ++i )
		Copy.emplace_back( Buffer.Samples[i] );

	Out.P50Us = Percentile( Copy , 0.50 );
	Out.P95Us = Percentile( Copy , 0.95 );
	Out.P99Us = Percentile( Copy , 0.99 );
	Out.P999Us = Percentile( Copy , 0.999 );
	Out.MaxUs = *std::max_element( Copy.begin() , Copy.end() );
	return Out;
}

auto CStageProfiler::SetBudgetUs( EStage Stage , double BudgetUs ) -> void
{
	m_BudgetsUs[static_cast<size_t>( Stage )] = std::max( 0.0 , BudgetUs );
}

auto CStageProfiler::IsWithinBudget( EStage Stage , double DurationUs ) const -> bool
{
	const double Budget = m_BudgetsUs[static_cast<size_t>( Stage )];
	return Budget <= 0.0 || DurationUs <= Budget;
}
