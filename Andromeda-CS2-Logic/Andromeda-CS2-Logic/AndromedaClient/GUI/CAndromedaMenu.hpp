#pragma once

#include <Common/Common.hpp>

class CAndromedaMenu
{
public:
	enum class EMenuTab : int
	{
		AIMBOT ,
		AUTOWALL ,
		BACKTRACK ,
		ANTIAIM ,
		EXPLOITS ,
		RESOLVER ,
		VISUALS ,
		COLORS ,
		SETTINGS
	};

public:
	auto OnRenderMenu() -> void;

private:
	auto RenderTopBar() -> void;
	auto RenderSideBar() -> void;
	auto RenderTabContent() -> void;

	auto RenderAimbotTab() -> void;
	auto RenderAutowallTab() -> void;
	auto RenderBacktrackTab() -> void;
	auto RenderAntiAimTab() -> void;
	auto RenderExploitsTab() -> void;
	auto RenderResolverTab() -> void;
	auto RenderVisualsTab() -> void;
	auto RenderColorsTab() -> void;
	auto RenderSettingsTab() -> void;

private:
	auto RenderSectionHeader( const char* szTitle , const char* szSubtitle ) -> void;
	auto RenderNavButton( const char* szLabel , EMenuTab Tab ) -> bool;

	auto RenderCheckBox( const char* szLabel , const char* szId , bool* pValue ) -> void;
	auto RenderSliderInt( const char* szLabel , const char* szId , int* pValue , int iMin , int iMax ) -> void;
	auto RenderSliderFloat( const char* szLabel , const char* szId , float* pValue , float fMin , float fMax ) -> void;
	auto RenderComboBox( const char* szLabel , const char* szId , int* pValue , const char** pItems , int iCount ) -> bool;
	auto RenderColorEdit( const char* szLabel , const char* szId , float* pValue ) -> void;

private:
	EMenuTab m_CurrentTab = EMenuTab::AIMBOT;
};

auto GetAndromedaMenu() -> CAndromedaMenu*;
