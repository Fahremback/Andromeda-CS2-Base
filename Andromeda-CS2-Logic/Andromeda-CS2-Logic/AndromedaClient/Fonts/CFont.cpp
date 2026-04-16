#include "CFont.hpp"

#include <cstring>

namespace
{
	auto ApplyAlignment( ImVec2& pos , const ImVec2& textSize , int flags ) -> void
	{
		if ( flags & FW1_CENTER )
			pos.x -= textSize.x * 0.5f;
		else if ( flags & FW1_RIGHT )
			pos.x -= textSize.x;

		if ( flags & FW1_VCENTER )
			pos.y -= textSize.y * 0.5f;
		else if ( flags & FW1_BOTTOM )
			pos.y -= textSize.y;
	}
}

auto CFont::InitFont( std::wstring Name , float FontSize ) -> void
{
	m_FontSize = FontSize;
	m_FontName = Name;
}

auto CFont::DrawString( int x , int y , ImColor Color , int Flags , const char* fmt , ... ) -> void
{
	char Buffer[g_BufferSize] = { 0 };

	va_list va_alist;
	va_start( va_alist , fmt );
	vsnprintf( Buffer , sizeof( Buffer ) , fmt , va_alist );
	va_end( va_alist );

	if ( Buffer[0] == '\0' )
		return;

	ImDrawList* pDrawList = ImGui::GetBackgroundDrawList();
	ImFont* pFont = ImGui::GetFont();
	const float flFontSize = m_FontSize > 0.f ? m_FontSize : ImGui::GetFontSize();
	const ImVec2 textSize = pFont->CalcTextSizeA( flFontSize , FLT_MAX , 0.f , Buffer );

	ImVec2 basePos( static_cast<float>( x ) , static_cast<float>( y ) );
	ApplyAlignment( basePos , textSize , Flags );

	const ImU32 textColor = ImGui::ColorConvertFloat4ToU32( Color.Value );
	const ImU32 outlineColor = IM_COL32( 0 , 0 , 0 , static_cast<int>( Color.Value.w * 255.f ) );

	pDrawList->AddText( pFont , flFontSize , ImVec2( basePos.x + 1.f , basePos.y + 1.f ) , outlineColor , Buffer );
	pDrawList->AddText( pFont , flFontSize , ImVec2( basePos.x - 1.f , basePos.y + 1.f ) , outlineColor , Buffer );
	pDrawList->AddText( pFont , flFontSize , ImVec2( basePos.x - 1.f , basePos.y - 1.f ) , outlineColor , Buffer );
	pDrawList->AddText( pFont , flFontSize , ImVec2( basePos.x + 1.f , basePos.y - 1.f ) , outlineColor , Buffer );
	pDrawList->AddText( pFont , flFontSize , basePos , textColor , Buffer );
}

auto CFont::DrawString( ImVec2 Pos , ImColor Color , int Flags , const char* fmt , ... ) -> void
{
	char Buffer[g_BufferSize] = { 0 };

	va_list va_alist;
	va_start( va_alist , fmt );
	vsnprintf( Buffer , sizeof( Buffer ) , fmt , va_alist );
	va_end( va_alist );

	DrawString( static_cast<int>( Pos.x ) , static_cast<int>( Pos.y ) , Color , Flags , Buffer );
}

auto CFont::GetStringLayoutRect( int Flags , FW1_RECTF* pRECTF , const char* fmt , ... ) -> FW1_RECTF
{
	(void)Flags;

	char Buffer[g_BufferSize] = { 0 };

	va_list va_alist;
	va_start( va_alist , fmt );
	vsnprintf( Buffer , sizeof( Buffer ) , fmt , va_alist );
	va_end( va_alist );

	const ImVec2 textSize = ImGui::CalcTextSize( Buffer );

	FW1_RECTF rect{};

	if ( pRECTF )
		rect = *pRECTF;

	rect.Right = rect.Left + textSize.x;
	rect.Bottom = rect.Top + textSize.y;

	return rect;
}
