#include "LatencyPipeline.hpp"

#include <immintrin.h>
#include <intrin.h>

static CLatencyPipeline g_LatencyPipeline{};

namespace
{
	auto SupportsAVX512() -> bool
	{
		int CpuInfo[4]{};
		__cpuidex( CpuInfo , 7 , 0 );
		return ( CpuInfo[1] & ( 1 << 16 ) ) != 0;
	}

	auto SupportsAVX2() -> bool
	{
		int CpuInfo[4]{};
		__cpuidex( CpuInfo , 7 , 0 );
		return ( CpuInfo[1] & ( 1 << 5 ) ) != 0;
	}

	auto SupportsSSE42() -> bool
	{
		int CpuInfo[4]{};
		__cpuidex( CpuInfo , 1 , 0 );
		return ( CpuInfo[2] & ( 1 << 20 ) ) != 0;
	}
}

auto CLatencyPipeline::Start( InputCallback_t Callback ) -> void
{
	m_InputCallback = std::move( Callback );

	m_Profiler.SetBudgetUs( CStageProfiler::EStage::INPUT , 300.0 );
	m_Profiler.SetBudgetUs( CStageProfiler::EStage::SIMULATION , 1000.0 );
	m_Profiler.SetBudgetUs( CStageProfiler::EStage::RENDER_SUBMIT , 200.0 );

	m_Running.store( true , std::memory_order_release );
	m_InputThread = std::jthread( [this]()
	{
		RunInputLoop();
	} );
}

auto CLatencyPipeline::Stop() -> void
{
	m_Running.store( false , std::memory_order_release );
	if ( m_InputThread.joinable() )
		m_InputThread.join();
}

auto CLatencyPipeline::RunInputLoop() -> void
{
	while ( m_Running.load( std::memory_order_acquire ) )
	{
		InputEvent_t Event{};
		if ( m_InputQueue.Pop( Event ) )
		{
			if ( m_InputCallback )
				m_InputCallback( Event );
		}
		else
		{
			std::this_thread::yield();
		}
	}
}

auto CLatencyPipeline::EnqueueInput( const InputEvent_t& Event ) -> bool
{
	return m_InputQueue.Push( Event );
}

auto CLatencyPipeline::BeginStage( CStageProfiler::EStage Stage ) -> void
{
	m_StageBegin[static_cast<size_t>( Stage )] = std::chrono::high_resolution_clock::now();
}

auto CLatencyPipeline::EndStage( CStageProfiler::EStage Stage ) -> void
{
	const auto End = std::chrono::high_resolution_clock::now();
	const auto Begin = m_StageBegin[static_cast<size_t>( Stage )];
	const double Us = static_cast<double>( std::chrono::duration_cast<std::chrono::microseconds>( End - Begin ).count() );

	m_Profiler.PushSample( Stage , Us );
	NextFrameBuffer().StageDurationsUs[static_cast<size_t>( Stage )] = Us;
}

auto CLatencyPipeline::NextFrameBuffer() -> FrameRecord_t&
{
	const uint64_t Frame = m_FrameId.fetch_add( 1 , std::memory_order_relaxed ) + 1;
	const size_t NextWrite = ( m_WriteIndex.load( std::memory_order_relaxed ) + 1 ) % m_FrameBuffers.size();
	m_WriteIndex.store( NextWrite , std::memory_order_release );
	m_ReadIndex.store( NextWrite , std::memory_order_release );

	auto& Buffer = m_FrameBuffers[NextWrite];
	Buffer.FrameId = Frame;
	return Buffer;
}

auto CLatencyPipeline::GetReadOnlyFrameBuffer() const -> const FrameRecord_t&
{
	return m_FrameBuffers[m_ReadIndex.load( std::memory_order_acquire )];
}

auto CLatencyPipeline::GetProfiler() -> CStageProfiler&
{
	return m_Profiler;
}

auto CLatencyPipeline::GetBestSIMDPathName() const -> const char*
{
	if ( SupportsAVX512() )
		return "AVX-512";
	if ( SupportsAVX2() )
		return "AVX2";
	if ( SupportsSSE42() )
		return "SSE4.2";
	return "SCALAR";
}

auto GetLatencyPipeline() -> CLatencyPipeline*
{
	return &g_LatencyPipeline;
}
