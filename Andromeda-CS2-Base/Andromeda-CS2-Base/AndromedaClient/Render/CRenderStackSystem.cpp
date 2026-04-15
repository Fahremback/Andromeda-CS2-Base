#include "CRenderStackSystem.hpp"
#include "CRender.hpp"

#include <cstdarg>
#include <cstdio>

static CRenderStackSystem g_CRenderStackSystem{};
static thread_local CRenderStackSystem::RenderCommands_t g_TLSUpdateBuffer{};

auto CRenderStackSystem::ReserveIfNeeded() -> void
{
	if ( g_TLSUpdateBuffer.capacity() < g_DefaultReserve )
		g_TLSUpdateBuffer.reserve( g_DefaultReserve );

	if ( m_BackBuffer.capacity() < g_DefaultReserve )
		m_BackBuffer.reserve( g_DefaultReserve );

	if ( m_RenderBuffer.capacity() < g_DefaultReserve )
		m_RenderBuffer.reserve( g_DefaultReserve );
}

auto CRenderStackSystem::PushCommand( const RenderCommand_t& Cmd ) -> void
{
	ReserveIfNeeded();
	g_TLSUpdateBuffer.emplace_back( Cmd );
}

auto CRenderStackSystem::DrawLine( const ImVec2& Start , const ImVec2& End , const ImColor& Color , const float thickness ) -> void
{
	RenderCommand_t Cmd{};
	Cmd.Type = ERenderCommandType::DRAW_LINE;
	Cmd.Pos0 = Start;
	Cmd.Pos1 = End;
	Cmd.Color = Color;
	Cmd.Float0 = thickness;
	PushCommand( Cmd );
}

auto CRenderStackSystem::DrawBox( const ImVec2& Min , const ImVec2& Max , const ImColor& Color ) -> void
{
	RenderCommand_t Cmd{};
	Cmd.Type = ERenderCommandType::DRAW_BOX;
	Cmd.Pos0 = Min;
	Cmd.Pos1 = Max;
	Cmd.Color = Color;
	PushCommand( Cmd );
}

auto CRenderStackSystem::DrawOutlineBox( const ImVec2& Min , const ImVec2& Max , const ImColor& Color ) -> void
{
	RenderCommand_t Cmd{};
	Cmd.Type = ERenderCommandType::DRAW_OUTLINE_BOX;
	Cmd.Pos0 = Min;
	Cmd.Pos1 = Max;
	Cmd.Color = Color;
	PushCommand( Cmd );
}

auto CRenderStackSystem::DrawCoalBox( const ImVec2& Min , const ImVec2& Max , const ImColor& Color ) -> void
{
	RenderCommand_t Cmd{};
	Cmd.Type = ERenderCommandType::DRAW_COAL_BOX;
	Cmd.Pos0 = Min;
	Cmd.Pos1 = Max;
	Cmd.Color = Color;
	PushCommand( Cmd );
}

auto CRenderStackSystem::DrawOutlineCoalBox( const ImVec2& Min , const ImVec2& Max , const ImColor& Color ) -> void
{
	RenderCommand_t Cmd{};
	Cmd.Type = ERenderCommandType::DRAW_OUTLINE_COAL_BOX;
	Cmd.Pos0 = Min;
	Cmd.Pos1 = Max;
	Cmd.Color = Color;
	PushCommand( Cmd );
}

auto CRenderStackSystem::DrawFillBox( const ImVec2& Min , const ImVec2& Max , const ImColor& Color ) -> void
{
	RenderCommand_t Cmd{};
	Cmd.Type = ERenderCommandType::DRAW_FILL_BOX;
	Cmd.Pos0 = Min;
	Cmd.Pos1 = Max;
	Cmd.Color = Color;
	PushCommand( Cmd );
}

auto CRenderStackSystem::DrawRectFilledMultiColor( const ImVec2& Min , const ImVec2& Max , const ImU32 col_upr_left , const ImU32 col_upr_right , const ImU32 col_bot_right , const ImU32 col_bot_left ) -> void
{
	RenderCommand_t Cmd{};
	Cmd.Type = ERenderCommandType::DRAW_RECT_FILLED_MULTI_COLOR;
	Cmd.Pos0 = Min;
	Cmd.Pos1 = Max;
	Cmd.U320 = col_upr_left;
	Cmd.U321 = col_upr_right;
	Cmd.U322 = col_bot_right;
	Cmd.U323 = col_bot_left;
	PushCommand( Cmd );
}

auto CRenderStackSystem::DrawCircle( const ImVec2& Center , float radius , const ImColor& Color ) -> void
{
	RenderCommand_t Cmd{};
	Cmd.Type = ERenderCommandType::DRAW_CIRCLE;
	Cmd.Pos0 = Center;
	Cmd.Float0 = radius;
	Cmd.Color = Color;
	PushCommand( Cmd );
}

auto CRenderStackSystem::DrawCircleFilled( const ImVec2& Center , const float radius , const ImColor& Color ) -> void
{
	RenderCommand_t Cmd{};
	Cmd.Type = ERenderCommandType::DRAW_CIRCLE_FILLED;
	Cmd.Pos0 = Center;
	Cmd.Float0 = radius;
	Cmd.Color = Color;
	PushCommand( Cmd );
}

auto CRenderStackSystem::DrawCircle3D( const Vector3& Center , float radius , const ImColor& Color ) -> void
{
	RenderCommand_t Cmd{};
	Cmd.Type = ERenderCommandType::DRAW_CIRCLE_3D;
	Cmd.Pos3D = Center;
	Cmd.Float0 = radius;
	Cmd.Color = Color;
	PushCommand( Cmd );
}

auto CRenderStackSystem::DrawTriangleFilled( const ImVec2& Center , const ImVec2& pos1 , const ImVec2& pos2 , const ImColor& Color ) -> void
{
	RenderCommand_t Cmd{};
	Cmd.Type = ERenderCommandType::DRAW_TRIANGLE_FILLED;
	Cmd.Pos0 = Center;
	Cmd.Pos1 = pos1;
	Cmd.Pos2 = pos2;
	Cmd.Color = Color;
	PushCommand( Cmd );
}

auto CRenderStackSystem::DrawString( CFont* pFont , const int X , const int Y , const int Flags , const ImColor& Color , const char* fmt , ... ) -> void
{
	char Buffer[g_BufferSize] = { 0 };

	va_list va_alist;
	va_start( va_alist , fmt );
	vsnprintf( Buffer , sizeof( Buffer ) , fmt , va_alist );
	va_end( va_alist );

	RenderCommand_t Cmd{};
	Cmd.Type = ERenderCommandType::DRAW_STRING;
	Cmd.pFont = pFont;
	Cmd.Int0 = X;
	Cmd.Int1 = Y;
	Cmd.Int2 = Flags;
	Cmd.Color = Color;
	std::snprintf( Cmd.Text.data() , Cmd.Text.size() , "%s" , Buffer );
	PushCommand( Cmd );
}

auto CRenderStackSystem::DrawString( CFont* pFont , const ImVec2& Pos , const int Flags , const ImColor& Color , const char* fmt , ... ) -> void
{
	char Buffer[g_BufferSize] = { 0 };

	va_list va_alist;
	va_start( va_alist , fmt );
	vsnprintf( Buffer , sizeof( Buffer ) , fmt , va_alist );
	va_end( va_alist );

	DrawString( pFont , static_cast<int>( Pos.x ) , static_cast<int>( Pos.y ) , Flags , Color , Buffer );
}

auto CRenderStackSystem::OnClientOutput() -> void
{
	std::scoped_lock lock( m_Lock );
	ReserveIfNeeded();

	m_BackBuffer.clear();
	m_BackBuffer.swap( g_TLSUpdateBuffer );
	m_bRenderBufferReady.store( true , std::memory_order_release );
}

auto CRenderStackSystem::ExecuteCommand( const RenderCommand_t& Cmd ) -> void
{
	switch ( Cmd.Type )
	{
		case ERenderCommandType::DRAW_LINE:
			GetRender()->DrawLine( Cmd.Pos0 , Cmd.Pos1 , Cmd.Color , Cmd.Float0 );
			break;
		case ERenderCommandType::DRAW_BOX:
			GetRender()->DrawBox( Cmd.Pos0 , Cmd.Pos1 , Cmd.Color );
			break;
		case ERenderCommandType::DRAW_OUTLINE_BOX:
			GetRender()->DrawOutlineBox( Cmd.Pos0 , Cmd.Pos1 , Cmd.Color );
			break;
		case ERenderCommandType::DRAW_COAL_BOX:
			GetRender()->DrawCoalBox( Cmd.Pos0 , Cmd.Pos1 , Cmd.Color );
			break;
		case ERenderCommandType::DRAW_OUTLINE_COAL_BOX:
			GetRender()->DrawOutlineCoalBox( Cmd.Pos0 , Cmd.Pos1 , Cmd.Color );
			break;
		case ERenderCommandType::DRAW_FILL_BOX:
			GetRender()->DrawFillBox( Cmd.Pos0 , Cmd.Pos1 , Cmd.Color );
			break;
		case ERenderCommandType::DRAW_RECT_FILLED_MULTI_COLOR:
			GetRender()->DrawRectFilledMultiColor( Cmd.Pos0 , Cmd.Pos1 , Cmd.U320 , Cmd.U321 , Cmd.U322 , Cmd.U323 );
			break;
		case ERenderCommandType::DRAW_CIRCLE:
			GetRender()->DrawCircle( Cmd.Pos0 , Cmd.Float0 , Cmd.Color );
			break;
		case ERenderCommandType::DRAW_CIRCLE_FILLED:
			GetRender()->DrawCircleFilled( Cmd.Pos0 , Cmd.Float0 , Cmd.Color );
			break;
		case ERenderCommandType::DRAW_CIRCLE_3D:
			GetRender()->DrawCircle3D( Cmd.Pos3D , Cmd.Float0 , Cmd.Color );
			break;
		case ERenderCommandType::DRAW_TRIANGLE_FILLED:
			GetRender()->DrawTriangleFilled( Cmd.Pos0 , Cmd.Pos1 , Cmd.Pos2 , Cmd.Color );
			break;
		case ERenderCommandType::DRAW_STRING:
			if ( Cmd.pFont )
				Cmd.pFont->DrawString( Cmd.Int0 , Cmd.Int1 , Cmd.Color , Cmd.Int2 , "%s" , Cmd.Text.data() );
			break;
	}
}

auto CRenderStackSystem::OnRenderStack() -> void
{
	if ( m_bRenderBufferReady.exchange( false , std::memory_order_acq_rel ) )
	{
		std::scoped_lock lock( m_Lock );
		m_RenderBuffer.clear();
		m_RenderBuffer.swap( m_BackBuffer );
	}

	for ( const auto& Cmd : m_RenderBuffer )
		ExecuteCommand( Cmd );
}

auto GetRenderStackSystem() -> CRenderStackSystem*
{
	return &g_CRenderStackSystem;
}
