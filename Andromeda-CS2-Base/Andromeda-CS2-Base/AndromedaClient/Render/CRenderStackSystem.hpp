#pragma once

#include <vector>
#include <array>
#include <atomic>
#include <mutex>
#include <ImGui/imgui.h>

#include <Common/Common.hpp>
#include <CS2/SDK/Math/Vector3.hpp>
#include <AndromedaClient/Fonts/CFont.hpp>

class CRenderStackSystem final
{
public:
	enum class ERenderCommandType : uint8_t
	{
		DRAW_LINE,
		DRAW_BOX,
		DRAW_OUTLINE_BOX,
		DRAW_COAL_BOX,
		DRAW_OUTLINE_COAL_BOX,
		DRAW_FILL_BOX,
		DRAW_RECT_FILLED_MULTI_COLOR,
		DRAW_CIRCLE,
		DRAW_CIRCLE_FILLED,
		DRAW_CIRCLE_3D,
		DRAW_TRIANGLE_FILLED,
		DRAW_STRING,
	};

	struct RenderCommand_t
	{
		ERenderCommandType Type = ERenderCommandType::DRAW_LINE;

		ImVec2 Pos0{};
		ImVec2 Pos1{};
		ImVec2 Pos2{};
		Vector3 Pos3D{};

		ImColor Color{};
		float Float0 = 0.f;

		int Int0 = 0;
		int Int1 = 0;
		int Int2 = 0;
		int Int3 = 0;

		ImU32 U320 = 0;
		ImU32 U321 = 0;
		ImU32 U322 = 0;
		ImU32 U323 = 0;

		CFont* pFont = nullptr;
		std::array<char , 64> Text{};
	};

	using RenderCommands_t = std::vector<RenderCommand_t>;
	using Lock_t = std::mutex;

public:
	auto DrawLine( const ImVec2& Start , const ImVec2& End , const ImColor& Color , const float thickness = 1.f ) -> void;
	auto DrawBox( const ImVec2& Min , const ImVec2& Max , const ImColor& Color ) -> void;
	auto DrawOutlineBox( const ImVec2& Min , const ImVec2& Max , const ImColor& Color ) -> void;
	auto DrawCoalBox( const ImVec2& Min , const ImVec2& Max , const ImColor& Color ) -> void;
	auto DrawOutlineCoalBox( const ImVec2& Min , const ImVec2& Max , const ImColor& Color ) -> void;
	auto DrawFillBox( const ImVec2& Min , const ImVec2& Max , const ImColor& Color ) -> void;
	auto DrawRectFilledMultiColor( const ImVec2& Min , const ImVec2& Max , const ImU32 col_upr_left , const ImU32 col_upr_right , const ImU32 col_bot_right , const ImU32 col_bot_left ) -> void;
	auto DrawCircle( const ImVec2& Center , float radius , const ImColor& Color ) -> void;
	auto DrawCircleFilled( const ImVec2& Center , const float radius , const ImColor& Color ) -> void;
	auto DrawCircle3D( const Vector3& Center , float radius , const ImColor& Color ) -> void;
	auto DrawTriangleFilled( const ImVec2& Center , const ImVec2& pos1 , const ImVec2& pos2 , const ImColor& Color ) -> void;

public:
	auto DrawString( CFont* pFont , const int X , const int Y , const int Flags , const ImColor& Color , const char* fmt , ... ) -> void;
	auto DrawString( CFont* pFont , const ImVec2& Pos , const int Flags , const ImColor& Color , const char* fmt , ... ) -> void;

public:
	auto OnClientOutput() -> void;
	auto OnRenderStack() -> void;

private:
	auto ReserveIfNeeded() -> void;
	auto PushCommand( const RenderCommand_t& Cmd ) -> void;
	auto ExecuteCommand( const RenderCommand_t& Cmd ) -> void;

private:
	RenderCommands_t m_UpdateBuffer;
	RenderCommands_t m_BackBuffer;
	RenderCommands_t m_RenderBuffer;

	std::atomic_bool m_bRenderBufferReady = false;
	Lock_t m_Lock;

private:
	static constexpr auto g_BufferSize = 64;
	static constexpr size_t g_DefaultReserve = 4096;
};

auto GetRenderStackSystem() -> CRenderStackSystem*;
