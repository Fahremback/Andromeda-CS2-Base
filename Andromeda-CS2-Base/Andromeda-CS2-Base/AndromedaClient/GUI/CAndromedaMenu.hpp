#pragma once

#include <Common/Common.hpp>

class CAndromedaMenu final
{
public:
	auto OnRenderMenu() -> void;

private:
	enum class EMenuTab : int
	{
		Aimbot = 0 ,
		Autowall ,
		Backtrack ,
		AntiAim ,
		Exploits ,
		Resolver ,
		Visuals ,
		Colors ,
		Settings
	};

	EMenuTab m_CurrentTab{ EMenuTab::Aimbot };

	auto RenderTopBar() -> void;
	auto RenderSidebar() -> void;
	auto RenderContent() -> void;
	auto RenderAimbotTab() -> void;
	auto RenderAutowallTab() -> void;
	auto RenderBacktrackTab() -> void;
	auto RenderAntiAimTab() -> void;
	auto RenderExploitsTab() -> void;
	auto RenderResolverTab() -> void;
	auto RenderVisualsTab() -> void;
	auto RenderColorsTab() -> void;
	auto RenderSettingsTab() -> void;
	auto RenderSectionHeader( const char* szTitle , const char* szSubtitle ) -> void;
	auto RenderNavButton( const char* szLabel , EMenuTab Tab ) -> bool;

	auto RenderCheckBox( const char* szTitle , const char* szStrID , bool& SettingsItem ) -> bool;
	auto RenderComboBox( const char* szTitle , const char* szStrID , int& v , const char* Items[] , int ItemsCount ) -> bool;
	auto RenderColorEdit( const char* szTitle , const char* szStrID , float* Color ) -> bool;
	auto RenderSliderInt( const char* szTitle , const char* szStrID , int& Value , int Min , int Max ) -> bool;
	auto RenderSliderFloat( const char* szTitle , const char* szStrID , float& Value , float Min , float Max , const char* szFormat = "%.3f" ) -> bool;
};

auto GetAndromedaMenu() -> CAndromedaMenu*;
