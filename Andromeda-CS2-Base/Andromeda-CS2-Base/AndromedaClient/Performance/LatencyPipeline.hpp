#pragma once

#include <Common/Common.hpp>

#include "SPSCQueue.hpp"
#include "StageProfiler.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <functional>
#include <thread>

class CLatencyPipeline final
{
public:
	struct InputEvent_t
	{
		uint64_t Tick = 0;
		int DeltaX = 0;
		int DeltaY = 0;
		uint64_t TimestampNs = 0;
	};

	struct FrameRecord_t
	{
		uint64_t FrameId = 0;
		std::array<double , static_cast<size_t>( CStageProfiler::EStage::COUNT )> StageDurationsUs{};
	};

	using InputCallback_t = std::function<void( const InputEvent_t& )>;

public:
	auto Start( InputCallback_t Callback ) -> void;
	auto Stop() -> void;
	auto EnqueueInput( const InputEvent_t& Event ) -> bool;

	auto BeginStage( CStageProfiler::EStage Stage ) -> void;
	auto EndStage( CStageProfiler::EStage Stage ) -> void;

	auto NextFrameBuffer() -> FrameRecord_t&;
	auto GetReadOnlyFrameBuffer() const -> const FrameRecord_t&;
	auto GetProfiler() -> CStageProfiler&;

	auto GetBestSIMDPathName() const -> const char*;

private:
	auto RunInputLoop() -> void;

private:
	static constexpr size_t INPUT_QUEUE_CAPACITY = 4096;
	CSPSCQueue<InputEvent_t , INPUT_QUEUE_CAPACITY> m_InputQueue{};
	InputCallback_t m_InputCallback{};
	std::jthread m_InputThread{};
	std::atomic_bool m_Running = false;

	std::array<FrameRecord_t , 3> m_FrameBuffers{};
	std::atomic_uint64_t m_FrameId = 0;
	std::atomic_size_t m_WriteIndex = 0;
	std::atomic_size_t m_ReadIndex = 0;

	CStageProfiler m_Profiler{};
	std::array<std::chrono::high_resolution_clock::time_point , static_cast<size_t>( CStageProfiler::EStage::COUNT )> m_StageBegin{};
};

auto GetLatencyPipeline() -> CLatencyPipeline*;
