#include "CAndromedaMenu.hpp"

#include <Common/Include/XorStr/XorStr.hpp>
#include <AndromedaClient/Settings/Settings.hpp>
#include <AndromedaClient/CAndromedaGUI.hpp>
#include <AndromedaClient/Features/CPhysicsAligner.hpp>
#include <AndromedaClient/Settings/Settings.hpp>

static CAndromedaMenu g_CAndromedaMenu{};

auto CAndromedaMenu::OnRenderMenu() -> void
{
	const float MenuAlphaVal = static_cast<float>( MenuAlpha_Setting ) / 255.f;

	ImGui::PushStyleVar( ImGuiStyleVar_Alpha , MenuAlphaVal );
	ImGui::SetNextWindowSize( ImVec2( 1080.f , 700.f ) , ImGuiCond_FirstUseEver );

	if ( ImGui::Begin( XorStr( "Andromeda CS2" ) , 0 , ImGuiWindowFlags_NoCollapse ) )
	{
		RenderTopBar();

		ImGui::Columns( 2 , XorStr( "##MainColumns" ) , false );
		ImGui::SetColumnWidth( 0 , 200.f );

		RenderSideBar();

		ImGui::NextColumn();

		RenderTabContent();

		ImGui::Columns( 1 );
	}
	ImGui::End();
	ImGui::PopStyleVar();
}

auto CAndromedaMenu::RenderTopBar() -> void
{
	ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing , ImVec2( 0.f , 0.f ) );
	ImGui::BeginChild( XorStr( "##TopBar" ) , ImVec2( 0.f , 60.f ) , false );
	{
		ImGui::SetCursorPos( ImVec2( 20.f , 15.f ) );
		ImGui::TextColored( ImVec4( 0.13f , 0.43f , 0.82f , 1.0f ) , XorStr( "ANDROMEDA" ) );

		ImGui::SameLine();
		ImGui::SetCursorPosY( 24.f );
		ImGui::TextColored( ImVec4( 0.64f , 0.69f , 0.77f , 1.0f ) , XorStr( " | CS2 Internal Base" ) );
	}
	ImGui::EndChild();
	ImGui::PopStyleVar();
}

auto CAndromedaMenu::RenderSideBar() -> void
{
	ImGui::BeginChild( XorStr( "##SideBar" ) , ImVec2( 200.f , 0.f ) , false );
	{
		ImGui::Spacing();
		if ( RenderNavButton( XorStr( "Aimbot" ) , EMenuTab::AIMBOT ) ) m_CurrentTab = EMenuTab::AIMBOT;
		if ( RenderNavButton( XorStr( "Autowall" ) , EMenuTab::AUTOWALL ) ) m_CurrentTab = EMenuTab::AUTOWALL;
		if ( RenderNavButton( XorStr( "Backtrack" ) , EMenuTab::BACKTRACK ) ) m_CurrentTab = EMenuTab::BACKTRACK;
		if ( RenderNavButton( XorStr( "Anti-Aim" ) , EMenuTab::ANTIAIM ) ) m_CurrentTab = EMenuTab::ANTIAIM;
		if ( RenderNavButton( XorStr( "Exploits" ) , EMenuTab::EXPLOITS ) ) m_CurrentTab = EMenuTab::EXPLOITS;
		if ( RenderNavButton( XorStr( "Resolver" ) , EMenuTab::RESOLVER ) ) m_CurrentTab = EMenuTab::RESOLVER;
		if ( RenderNavButton( XorStr( "Visuals" ) , EMenuTab::VISUALS ) ) m_CurrentTab = EMenuTab::VISUALS;
		if ( RenderNavButton( XorStr( "Colors" ) , EMenuTab::COLORS ) ) m_CurrentTab = EMenuTab::COLORS;
		if ( RenderNavButton( XorStr( "Settings" ) , EMenuTab::SETTINGS ) ) m_CurrentTab = EMenuTab::SETTINGS;
	}
	ImGui::EndChild();
}

auto CAndromedaMenu::RenderTabContent() -> void
{
	ImGui::BeginChild( XorStr( "##TabContent" ) , ImVec2( 0.f , 0.f ) , false );
	{
		switch ( m_CurrentTab )
		{
		case EMenuTab::AIMBOT: RenderAimbotTab(); break;
		case EMenuTab::AUTOWALL: RenderAutowallTab(); break;
		case EMenuTab::BACKTRACK: RenderBacktrackTab(); break;
		case EMenuTab::ANTIAIM: RenderAntiAimTab(); break;
		case EMenuTab::EXPLOITS: RenderExploitsTab(); break;
		case EMenuTab::RESOLVER: RenderResolverTab(); break;
		case EMenuTab::VISUALS: RenderVisualsTab(); break;
		case EMenuTab::COLORS: RenderColorsTab(); break;
		case EMenuTab::SETTINGS: RenderSettingsTab(); break;
		}
	}
	ImGui::EndChild();
}

auto CAndromedaMenu::RenderAimbotTab() -> void
{
	RenderSectionHeader( XorStr( "Aimbot" ) , XorStr( "Fase 1: FOV Filter, Aimbot Math, Smooth/RCS, Hitchance" ) );

	ImGui::BeginChild( XorStr( "##Aimbot.Core" ) , ImVec2( 0.f , 0.f ) , true );
	RenderCheckBox( XorStr( "Enable Aimbot" ) , XorStr( "##Aimbot.Enabled" ) , &Aimbot_Enabled_Setting );
	RenderCheckBox( XorStr( "Trigger Bot" ) , XorStr( "##Aimbot.TriggerBot" ) , &Aimbot_TriggerBot_Setting );
	RenderSliderFloat( XorStr( "Smooth Aim" ) , XorStr( "##Aimbot.SmoothVal" ) , &Aimbot_Smooth_Setting , 1.0f , 100.0f );
	RenderSliderFloat( XorStr( "FOV" ) , XorStr( "##Aimbot.FOV" ) , &Aimbot_FOV_Setting , 0.0f , 180.0f );
	RenderSliderInt( XorStr( "Hitchance %" ) , XorStr( "##Aimbot.Hitchance" ) , &Aimbot_Hitchance_Setting , 0 , 100 );
	RenderSliderInt( XorStr( "Min Damage" ) , XorStr( "##Aimbot.MinDamage" ) , &Aimbot_MinDamage_Setting , 1 , 100 );

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	RenderSectionHeader( XorStr( "Recoil Control System (RCS)" ) , XorStr( "Controla o recuo da arma automaticamente" ) );
	RenderCheckBox( XorStr( "Enable RCS" ) , XorStr( "##Aimbot.RCS" ) , &Aimbot_RCS_Setting );
	RenderSliderFloat( XorStr( "RCS Scale X" ) , XorStr( "##Aimbot.RCSScaleX" ) , &Aimbot_RCSScaleX_Setting , 0.0f , 5.0f );
	RenderSliderFloat( XorStr( "RCS Scale Y" ) , XorStr( "##Aimbot.RCSScaleY" ) , &Aimbot_RCSScaleY_Setting , 0.0f , 5.0f );

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	RenderSectionHeader( XorStr( "Backtrack" ) , XorStr( "Atira nos fantasmas do passado" ) );
	RenderCheckBox( XorStr( "Enable Backtrack" ) , XorStr( "##Aimbot.Backtrack" ) , &Aimbot_Backtrack_Setting );
	RenderSliderInt( XorStr( "Backtrack Ticks" ) , XorStr( "##Aimbot.BacktrackTicks" ) , &Aimbot_BacktrackTicks_Setting , 1 , 14 );
	ImGui::EndChild();
}

auto CAndromedaMenu::RenderAutowallTab() -> void
{
	RenderSectionHeader( XorStr( "Autowall / Wallbang" ) , XorStr( "Fase 2: Calculo de penetracao e perda de dano por parede" ) );

	ImGui::BeginChild( XorStr( "##Autowall.Core" ) , ImVec2( 0.f , 0.f ) , true );
	RenderCheckBox( XorStr( "Enable Autowall" ) , XorStr( "##Autowall.Enabled" ) , &Autowall_Enabled_Setting );
	RenderSliderInt( XorStr( "Min Damage" ) , XorStr( "##Autowall.MinDamage" ) , &Autowall_MinDamage_Setting , 1 , 100 );
	ImGui::EndChild();
}

auto CAndromedaMenu::RenderBacktrackTab() -> void
{
	RenderSectionHeader( XorStr( "Backtrack" ) , XorStr( "Fase 3: Armazenamento e disparo em ticks passados" ) );

	ImGui::BeginChild( XorStr( "##Backtrack.Core" ) , ImVec2( 0.f , 0.f ) , true );
	RenderCheckBox( XorStr( "Enable Backtrack" ) , XorStr( "##Backtrack.Enabled" ) , &Aimbot_Backtrack_Setting );
	RenderSliderInt( XorStr( "History Ticks" ) , XorStr( "##Backtrack.Ticks" ) , &Aimbot_BacktrackTicks_Setting , 1 , 14 );
	ImGui::EndChild();
}

auto CAndromedaMenu::RenderAntiAimTab() -> void
{
	RenderSectionHeader( XorStr( "Anti-Aim" ) , XorStr( "Fase 4: Desync, Pitch AA, Yaw Jitter, LBY Breaker" ) );

	ImGui::BeginChild( XorStr( "##AntiAim.Core" ) , ImVec2( 0.f , 260.f ) , true );
	RenderCheckBox( XorStr( "Enable Anti-Aim" ) , XorStr( "##AntiAim.Enabled" ) , &AntiAim_Enabled_Setting );
	RenderCheckBox( XorStr( "Desync (Real/Fake)" ) , XorStr( "##AntiAim.Desync" ) , &AntiAim_Desync_Setting );
	RenderSliderFloat( XorStr( "Desync Strength" ) , XorStr( "##AntiAim.DesyncStr" ) , &AntiAim_DesyncStrength_Setting , 0.0f , 120.0f );

	const char* PitchItems[] = { "None" , "Down 89" , "Up 89" , "Zero" , "Fake Down" };
	RenderComboBox( XorStr( "Pitch" ) , XorStr( "##AntiAim.Pitch" ) , &AntiAim_Pitch_Setting , PitchItems , IM_ARRAYSIZE( PitchItems ) );

	RenderCheckBox( XorStr( "Yaw Jitter" ) , XorStr( "##AntiAim.YawJitter" ) , &AntiAim_YawJitter_Setting );
	RenderSliderFloat( XorStr( "Yaw Jitter Amount" ) , XorStr( "##AntiAim.YawJitterAmt" ) , &AntiAim_YawJitterAmount_Setting , 0.0f , 180.0f );
	RenderCheckBox( XorStr( "LBY Breaker" ) , XorStr( "##AntiAim.LBYBreaker" ) , &AntiAim_LBYBreaker_Setting );
	RenderSliderFloat( XorStr( "LBY Breaker Speed" ) , XorStr( "##AntiAim.LBYBreakerSpd" ) , &AntiAim_LBYBreakerSpeed_Setting , 0.0f , 120.0f );
	ImGui::EndChild();
}

auto CAndromedaMenu::RenderExploitsTab() -> void
{
	RenderSectionHeader( XorStr( "Network Exploits" ) , XorStr( "Fase 5: Fake Lag, Hide Shots, Tickbase Shift / Double Tap" ) );

	ImGui::BeginChild( XorStr( "##Exploits.Core" ) , ImVec2( 0.f , 0.f ) , true );
	RenderCheckBox( XorStr( "Fake Lag" ) , XorStr( "##Exploits.FakeLag" ) , &NetworkExploits_FakeLag_Setting );
	RenderSliderInt( XorStr( "Fake Lag Ticks" ) , XorStr( "##Exploits.FakeLagTicks" ) , &NetworkExploits_FakeLagTicks_Setting , 1 , 14 );
	RenderCheckBox( XorStr( "Hide Shots / Silent Aim" ) , XorStr( "##Exploits.HideShots" ) , &NetworkExploits_HideShots_Setting );
	RenderCheckBox( XorStr( "Tickbase Shift / Double Tap" ) , XorStr( "##Exploits.TickbaseShift" ) , &NetworkExploits_TickbaseShift_Setting );
	ImGui::EndChild();
}

auto CAndromedaMenu::RenderResolverTab() -> void
{
	RenderSectionHeader( XorStr( "Resolver" ) , XorStr( "Fase 6: Correcao de angulos fake do inimigo" ) );

	ImGui::BeginChild( XorStr( "##Resolver.Core" ) , ImVec2( 0.f , 0.f ) , true );
	RenderCheckBox( XorStr( "Enable Resolver" ) , XorStr( "##Resolver.Enabled" ) , &Resolver_Enabled_Setting );
	RenderCheckBox( XorStr( "Brute-Force Resolver" ) , XorStr( "##Resolver.BruteForce" ) , &Resolver_BruteForce_Setting );
	RenderCheckBox( XorStr( "Animation Resolve" ) , XorStr( "##Resolver.AnimationResolve" ) , &Resolver_AnimationResolve_Setting );
	RenderCheckBox( XorStr( "Desync Corrector" ) , XorStr( "##Resolver.DesyncCorrect" ) , &Resolver_DesyncCorrect_Setting );
	RenderCheckBox( XorStr( "Extrapolation" ) , XorStr( "##Resolver.Extrapolation" ) , &Resolver_Extrapolation_Setting );
	ImGui::EndChild();
}

auto CAndromedaMenu::RenderVisualsTab() -> void
{
	RenderSectionHeader( XorStr( "ESP" ) , XorStr( "Visuals: Box, Skeleton, Glow" ) );

	ImGui::BeginChild( XorStr( "##Visual.Core" ) , ImVec2( 0.f , 260.f ) , true );
	RenderCheckBox( XorStr( "Enable ESP" ) , XorStr( "##Visual.Active" ) , &Active_Setting );
	RenderCheckBox( XorStr( "Show Team" ) , XorStr( "##Visual.Team" ) , &Team_Setting );
	RenderCheckBox( XorStr( "Show Enemy" ) , XorStr( "##Visual.Enemy" ) , &Enemy_Setting );
	RenderCheckBox( XorStr( "Only Visible" ) , XorStr( "##Visual.OnlyVisible" ) , &OnlyVisible_Setting );
	RenderCheckBox( XorStr( "Player Box" ) , XorStr( "##Visual.PlayerBox" ) , &PlayerBox_Setting );

	const char* PlayerBoxTypeItems[] =
	{
		"Box" , "Outline Box" , "Coal Box" , "Outline Coal Box"
	};
	RenderComboBox( XorStr( "Box Type" ) , XorStr( "##Visual.PlayerBoxType" ) , &PlayerBoxType_Setting , PlayerBoxTypeItems , IM_ARRAYSIZE( PlayerBoxTypeItems ) );

	RenderCheckBox( XorStr( "Bone ESP" ) , XorStr( "##Visual.BoneESP" ) , &BoneESP_Setting );
	if ( BoneESP_Setting )
	{
		RenderCheckBox( XorStr( "Bone ESP Team" ) , XorStr( "##Visual.BoneESPTeam" ) , &BoneESPTeam_Setting );
		RenderCheckBox( XorStr( "Bone ESP Enemy" ) , XorStr( "##Visual.BoneESPEnemy" ) , &BoneESPEnemy_Setting );
	}

	RenderCheckBox( XorStr( "Glow" ) , XorStr( "##Visual.Glow" ) , &Glow_Setting );
	if ( Glow_Setting )
	{
		RenderCheckBox( XorStr( "Glow Team" ) , XorStr( "##Visual.GlowTeam" ) , &GlowTeam_Setting );
		RenderCheckBox( XorStr( "Glow Enemy" ) , XorStr( "##Visual.GlowEnemy" ) , &GlowEnemy_Setting );
	}
	ImGui::EndChild();
}

auto CAndromedaMenu::RenderColorsTab() -> void
{
	RenderSectionHeader( XorStr( "Cores" ) , XorStr( "Cores do ESP por time e visibilidade" ) );

	ImGui::BeginChild( XorStr( "##Colors.Primary" ) , ImVec2( 0.f , 0.f ) , true );
	RenderColorEdit( XorStr( "Player Box TT" ) , XorStr( "##Colors.Visual.PlayerBoxTT" ) , &PlayerBoxTT_Setting.x );
	RenderColorEdit( XorStr( "Player Box TT Visible" ) , XorStr( "##Colors.Visual.PlayerBoxTT_Visible" ) , &PlayerBoxTT_Visible_Setting.x );
	RenderColorEdit( XorStr( "Player Box CT" ) , XorStr( "##Colors.Visual.PlayerBoxCT" ) , &PlayerBoxCT_Setting.x );
	RenderColorEdit( XorStr( "Player Box CT Visible" ) , XorStr( "##Colors.Visual.PlayerBoxCT_Visible" ) , &PlayerBoxCT_Visible_Setting.x );
	RenderColorEdit( XorStr( "Bone ESP TT" ) , XorStr( "##Colors.Visual.BoneESPTT" ) , &BoneESPTT_Setting.x );
	RenderColorEdit( XorStr( "Bone ESP TT Visible" ) , XorStr( "##Colors.Visual.BoneESPTT_Visible" ) , &BoneESPTT_Visible_Setting.x );
	RenderColorEdit( XorStr( "Bone ESP CT" ) , XorStr( "##Colors.Visual.BoneESPCT" ) , &BoneESPCT_Setting.x );
	RenderColorEdit( XorStr( "Bone ESP CT Visible" ) , XorStr( "##Colors.Visual.BoneESPCT_Visible" ) , &BoneESPCT_Visible_Setting.x );
	RenderColorEdit( XorStr( "Glow TT" ) , XorStr( "##Colors.Visual.GlowTT" ) , &GlowTT_Setting.x );
	RenderColorEdit( XorStr( "Glow TT Visible" ) , XorStr( "##Colors.Visual.GlowTT_Visible" ) , &GlowTT_Visible_Setting.x );
	RenderColorEdit( XorStr( "Glow CT" ) , XorStr( "##Colors.Visual.GlowCT" ) , &GlowCT_Setting.x );
	RenderColorEdit( XorStr( "Glow CT Visible" ) , XorStr( "##Colors.Visual.GlowCT_Visible" ) , &GlowCT_Visible_Setting.x );
	ImGui::EndChild();
}

auto CAndromedaMenu::RenderSettingsTab() -> void
{
	RenderSectionHeader( XorStr( "Settings" ) , XorStr( "Configuracoes do menu e informacoes do sistema" ) );

	ImGui::BeginChild( XorStr( "##System.Menu" ) , ImVec2( 0.f , 160.f ) , true );
	RenderSliderInt( XorStr( "Menu Alpha" ) , XorStr( "##Menu.MenuAlpha" ) , &MenuAlpha_Setting , 100 , 255 );

	const char* MenuStyleItems[] =
	{
		"Indigo" , "Vermillion" , "Classic Steam"
	};
	if ( RenderComboBox( XorStr( "Menu Style" ) , XorStr( "##Menu.MenuStyle" ) , &MenuStyle_Setting , MenuStyleItems , IM_ARRAYSIZE( MenuStyleItems ) ) )
		GetAndromedaGUI()->UpdateStyle();
	ImGui::EndChild();

	ImGui::Spacing();
	ImGui::BulletText( "AVX-512 Support: %s" , CPhysicsAligner::HasAVX512() ? "YES" : "NO" );
	ImGui::BulletText( "Thread Count: %d" , CPhysicsAligner::GetOptimalThreadCount() );
}

auto CAndromedaMenu::RenderSectionHeader( const char* szTitle , const char* szSubtitle ) -> void
{
	ImGui::TextColored( ImVec4( 0.54f , 0.87f , 1.0f , 1.0f ) , szTitle );
	ImGui::TextColored( ImVec4( 0.64f , 0.69f , 0.77f , 1.0f ) , szSubtitle );
	ImGui::Separator();
}

auto CAndromedaMenu::RenderNavButton( const char* szLabel , EMenuTab Tab ) -> bool
{
	const bool bSelected = m_CurrentTab == Tab;

	if ( bSelected )
	{
		ImGui::PushStyleColor( ImGuiCol_Button , ImVec4( 0.13f , 0.43f , 0.82f , 1.0f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered , ImVec4( 0.16f , 0.50f , 0.90f , 1.0f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive , ImVec4( 0.12f , 0.38f , 0.72f , 1.0f ) );
	}
	else
	{
		ImGui::PushStyleColor( ImGuiCol_Button , ImVec4( 0.10f , 0.12f , 0.16f , 1.0f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered , ImVec4( 0.14f , 0.16f , 0.22f , 1.0f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive , ImVec4( 0.08f , 0.10f , 0.14f , 1.0f ) );
	}

	const bool bPressed = ImGui::Button( szLabel , ImVec2( -1.f , 40.f ) );

	ImGui::PopStyleColor( 3 );
	return bPressed;
}

auto CAndromedaMenu::RenderCheckBox( const char* szLabel , const char* szId , bool* pValue ) -> void
{
	ImGui::Text( szLabel );
	ImGui::SameLine( ImGui::GetWindowWidth() - 50.f );
	ImGui::Checkbox( szId , pValue );
}

auto CAndromedaMenu::RenderSliderInt( const char* szLabel , const char* szId , int* pValue , int iMin , int iMax ) -> void
{
	ImGui::Text( szLabel );
	ImGui::PushItemWidth( -1.f );
	ImGui::SliderInt( szId , pValue , iMin , iMax );
	ImGui::PopItemWidth();
}

auto CAndromedaMenu::RenderSliderFloat( const char* szLabel , const char* szId , float* pValue , float fMin , float fMax ) -> void
{
	ImGui::Text( szLabel );
	ImGui::PushItemWidth( -1.f );
	ImGui::SliderFloat( szId , pValue , fMin , fMax , XorStr( "%.1f" ) );
	ImGui::PopItemWidth();
}

auto CAndromedaMenu::RenderComboBox( const char* szLabel , const char* szId , int* pValue , const char** pItems , int iCount ) -> bool
{
	ImGui::Text( szLabel );
	ImGui::PushItemWidth( -1.f );
	const bool bChanged = ImGui::Combo( szId , pValue , pItems , iCount );
	ImGui::PopItemWidth();
	return bChanged;
}

auto CAndromedaMenu::RenderColorEdit( const char* szLabel , const char* szId , float* pValue ) -> void
{
	ImGui::Text( szLabel );
	ImGui::SameLine( ImGui::GetWindowWidth() - 50.f );
	ImGui::ColorEdit4( szId , pValue , ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel );
}

auto GetAndromedaMenu() -> CAndromedaMenu*
{
	return &g_CAndromedaMenu;
}
