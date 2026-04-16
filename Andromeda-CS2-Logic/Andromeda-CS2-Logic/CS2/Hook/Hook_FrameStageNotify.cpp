#include "Hook_FrameStageNotify.hpp"

#include <AndromedaClient/CAndromedaClient.hpp>
#include <AndromedaClient/Features/CPhysicsAligner.hpp>
#include <AndromedaClient/Features/CPhysicsSimulationMocks.hpp>

#include <GameClient/CL_Players.hpp>

auto Hook_FrameStageNotify( CSource2Client* pCSource2Client , int FrameStage ) -> void
{
	GetAndromedaClient()->OnFrameStageNotify( FrameStage );

	// FIX: Removed ApplyRenderSnapshot from FrameStageNotify.
	// Writing to pawn memory during match start (FRAME_RENDER_START) races with
	// game's entity construction/destruction, causing crashes.
	// Render angles are now set via ioState in CreateMove and consumed by the async thread.

	return FrameStageNotify_o( pCSource2Client , FrameStage );
}
