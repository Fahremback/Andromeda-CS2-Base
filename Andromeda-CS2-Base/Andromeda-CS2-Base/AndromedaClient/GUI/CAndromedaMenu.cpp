#include "CAndromedaMenu.hpp"

#include <ImGui/imgui.h>

#include <AndromedaClient/Settings/Settings.hpp>
#include <AndromedaClient/CAndromedaGUI.hpp>
#include <AndromedaClient/Features/CPhysicsAligner.hpp>
#include <string>

static CAndromedaMenu g_CAndromedaMenu{};

auto CAndromedaMenu::OnRenderMenu() -> void
{
	const float MenuAlpha = static_cast<float>( Settings::Menu::MenuAlpha ) / 255.f;

	ImGui::PushStyleVar( ImGuiStyleVar_Alpha , MenuAlpha );
	ImGui::SetNextWindowSize( ImVec2( 1080.f , 700.f ) , ImGuiCond_FirstUseEver );

	if ( ImGui::Begin( XorStr( "Andromeda // Neural Performance Center" ) , 0 , ImGuiWindowFlags_NoCollapse ) )
	{
		RenderTopBar();

		ImGui::BeginChild( XorStr( "##Menu.Sidebar" ) , ImVec2( 260.f , 0.f ) , true );
		RenderSidebar();
		ImGui::EndChild();

		ImGui::SameLine();

		ImGui::BeginChild( XorStr( "##Menu.Content" ) , ImVec2( 0.f , 0.f ) , true );
		RenderContent();
		ImGui::EndChild();
	}

	ImGui::End();

	ImGui::PopStyleVar();
}

auto CAndromedaMenu::RenderTopBar() -> void
{
	ImGui::TextColored( ImVec4( 0.42f , 0.82f , 1.00f , 1.00f ) , XorStr( "Andromeda CS2 Base" ) );
	ImGui::SameLine();
	ImGui::TextDisabled( XorStr( "| Extreme Optimization UI • Modular by Design" ) );
	ImGui::Separator();
}

auto CAndromedaMenu::RenderSidebar() -> void
{
	ImGui::TextColored( ImVec4( 0.65f , 0.89f , 1.00f , 1.00f ) , XorStr( "Modules" ) );
	ImGui::Spacing();

	RenderNavButton( XorStr( "Collision Optimizer" ) , EMenuTab::CollisionOptimizer );
	RenderNavButton( XorStr( "Visual Stack" ) , EMenuTab::Visuals );
	RenderNavButton( XorStr( "Color Pipeline" ) , EMenuTab::Colors );
	RenderNavButton( XorStr( "System / Menu" ) , EMenuTab::Menu );

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();
	ImGui::TextWrapped( XorStr( "Interface pronta para receber novas features sem reescrever o layout principal." ) );
}

auto CAndromedaMenu::RenderContent() -> void
{
	switch ( m_CurrentTab )
	{
	case EMenuTab::CollisionOptimizer:
		RenderCollisionOptimizerTab();
		break;
	case EMenuTab::Visuals:
		RenderVisualsTab();
		break;
	case EMenuTab::Colors:
		RenderColorsTab();
		break;
	case EMenuTab::Menu:
		RenderMenuTab();
		break;
	default:
		break;
	}
}

auto CAndromedaMenu::RenderCollisionOptimizerTab() -> void
{
	auto& alignmentConfig = CPhysicsAligner::GetConfig();

	RenderSectionHeader( XorStr( "Collision Optimizer" ) , XorStr( "Pipeline de combate vetorizado com foco em baixa latencia" ) );

	ImGui::BeginChild( XorStr( "##CollisionOptimizer.Core" ) , ImVec2( 0.f , 220.f ) , true );
	RenderCheckBox( XorStr( "Enable Collision Optimizer" ) , XorStr( "##CollisionOptimizer.Enabled" ) , alignmentConfig.enabled );
	RenderCheckBox( XorStr( "Trigger Interaction" ) , XorStr( "##CollisionOptimizer.TriggerInteraction" ) , alignmentConfig.triggerInteraction );
	RenderCheckBox( XorStr( "Phase Bypass" ) , XorStr( "##CollisionOptimizer.PhaseBypass" ) , alignmentConfig.phaseBypass );
	RenderCheckBox( XorStr( "Vibration Damping" ) , XorStr( "##CollisionOptimizer.VibrationDamping" ) , alignmentConfig.vibrationDamping );
	RenderSliderInt( XorStr( "Anchor Point" ) , XorStr( "##CollisionOptimizer.AnchorPoint" ) , alignmentConfig.anchorPoint , 0 , 19 );
	RenderSliderInt( XorStr( "Min Damage" ) , XorStr( "##CollisionOptimizer.MinDamage" ) , alignmentConfig.minDamage , 1 , 100 );
	RenderSliderFloat( XorStr( "Smooth" ) , XorStr( "##CollisionOptimizer.Smooth" ) , alignmentConfig.smooth , 1.0f , 100.0f );
	RenderSliderFloat( XorStr( "FOV" ) , XorStr( "##CollisionOptimizer.FOV" ) , alignmentConfig.fov , 0.0f , 180.0f );

	if ( alignmentConfig.vibrationDamping )
	{
		RenderSliderFloat( XorStr( "Vibration X" ) , XorStr( "##CollisionOptimizer.VDX" ) , alignmentConfig.vibrationDampingX , -10.0f , 10.0f );
		RenderSliderFloat( XorStr( "Vibration Y" ) , XorStr( "##CollisionOptimizer.VDY" ) , alignmentConfig.vibrationDampingY , -10.0f , 10.0f );
	}
	ImGui::EndChild();

	ImGui::Spacing();
	RenderTechCard(
		XorStr( "SIMD Batch W2S (AVX-512)" ) ,
		XorStr( "Projecao de todos os ossos de multiplos players em lote vetorial unico." ) ,
		XorStr( "Elimina loop escalar para varredura agressiva de alvos." ) );
	RenderTechCard(
		XorStr( "SoA ActiveParticle Cache + rsqrt14" ) ,
		XorStr( "Posicoes contiguas no cache e normalizacao rapida do vetor de mira." ) ,
		XorStr( "Menos cache misses e queda grande no custo de ciclos." ) );
}

auto CAndromedaMenu::RenderVisualsTab() -> void
{
	RenderSectionHeader( XorStr( "Visual Stack" ) , XorStr( "Overlay organizado por blocos para expansao futura" ) );

	ImGui::BeginChild( XorStr( "##Visual.Core" ) , ImVec2( 0.f , 260.f ) , true );
	RenderCheckBox( XorStr( "Active" ) , XorStr( "##Visual.Active" ) , Settings::Visual::Active );
	RenderCheckBox( XorStr( "Team" ) , XorStr( "##Visual.Team" ) , Settings::Visual::Team );
	RenderCheckBox( XorStr( "Enemy" ) , XorStr( "##Visual.Enemy" ) , Settings::Visual::Enemy );
	RenderCheckBox( XorStr( "OnlyVisible" ) , XorStr( "##Visual.OnlyVisible" ) , Settings::Visual::OnlyVisible );
	RenderCheckBox( XorStr( "Player Box" ) , XorStr( "##Visual.PlayerBox" ) , Settings::Visual::PlayerBox );

	const char* PlayerBoxTypeItems[] =
	{
		"Box" , "Outline Box" , "Coal Box" , "Outline Coal Box"
	};
	RenderComboBox( XorStr( "PlayerBox Type" ) , XorStr( "##Visual.PlayerBoxType" ) , Settings::Visual::PlayerBoxType , PlayerBoxTypeItems , IM_ARRAYSIZE( PlayerBoxTypeItems ) );

	RenderCheckBox( XorStr( "Bone ESP" ) , XorStr( "##Visual.BoneESP" ) , Settings::Visual::BoneESP );
	if ( Settings::Visual::BoneESP )
	{
		RenderCheckBox( XorStr( "Bone ESP Team" ) , XorStr( "##Visual.BoneESPTeam" ) , Settings::Visual::BoneESPTeam );
		RenderCheckBox( XorStr( "Bone ESP Enemy" ) , XorStr( "##Visual.BoneESPEnemy" ) , Settings::Visual::BoneESPEnemy );
	}

	RenderCheckBox( XorStr( "Glow" ) , XorStr( "##Visual.Glow" ) , Settings::Visual::Glow );
	if ( Settings::Visual::Glow )
	{
		RenderCheckBox( XorStr( "Glow Team" ) , XorStr( "##Visual.GlowTeam" ) , Settings::Visual::GlowTeam );
		RenderCheckBox( XorStr( "Glow Enemy" ) , XorStr( "##Visual.GlowEnemy" ) , Settings::Visual::GlowEnemy );
	}
	ImGui::EndChild();

	ImGui::Spacing();
	RenderTechCard(
		XorStr( "FMA Projection Kernel" ) ,
		XorStr( "Multiplicacao de matriz com _mm512_fmadd_ps para projeção consistente." ) ,
		XorStr( "Precisao de hardware sem custo extra de pipeline." ) );
}

auto CAndromedaMenu::RenderColorsTab() -> void
{
	RenderSectionHeader( XorStr( "Color Pipeline" ) , XorStr( "Ajuste visual rapido para cada feature de ESP" ) );

	ImGui::BeginChild( XorStr( "##Colors.Primary" ) , ImVec2( 0.f , 0.f ) , true );
	RenderColorEdit( XorStr( "Player Box TT" ) , XorStr( "##Colors.Visual.PlayerBoxTT" ) , &Settings::Colors::Visual::PlayerBoxTT.x );
	RenderColorEdit( XorStr( "Player Box TT Visible" ) , XorStr( "##Colors.Visual.PlayerBoxTT_Visible" ) , &Settings::Colors::Visual::PlayerBoxTT_Visible.x );
	RenderColorEdit( XorStr( "Player Box CT" ) , XorStr( "##Colors.Visual.PlayerBoxCT" ) , &Settings::Colors::Visual::PlayerBoxCT.x );
	RenderColorEdit( XorStr( "Player Box CT Visible" ) , XorStr( "##Colors.Visual.PlayerBoxCT_Visible" ) , &Settings::Colors::Visual::PlayerBoxCT_Visible.x );
	RenderColorEdit( XorStr( "Bone ESP TT" ) , XorStr( "##Colors.Visual.BoneESPTT" ) , &Settings::Colors::Visual::BoneESPTT.x );
	RenderColorEdit( XorStr( "Bone ESP TT Visible" ) , XorStr( "##Colors.Visual.BoneESPTT_Visible" ) , &Settings::Colors::Visual::BoneESPTT_Visible.x );
	RenderColorEdit( XorStr( "Bone ESP CT" ) , XorStr( "##Colors.Visual.BoneESPCT" ) , &Settings::Colors::Visual::BoneESPCT.x );
	RenderColorEdit( XorStr( "Bone ESP CT Visible" ) , XorStr( "##Colors.Visual.BoneESPCT_Visible" ) , &Settings::Colors::Visual::BoneESPCT_Visible.x );
	RenderColorEdit( XorStr( "Glow TT" ) , XorStr( "##Colors.Visual.GlowTT" ) , &Settings::Colors::Visual::GlowTT.x );
	RenderColorEdit( XorStr( "Glow TT Visible" ) , XorStr( "##Colors.Visual.GlowTT_Visible" ) , &Settings::Colors::Visual::GlowTT_Visible.x );
	RenderColorEdit( XorStr( "Glow CT" ) , XorStr( "##Colors.Visual.GlowCT" ) , &Settings::Colors::Visual::GlowCT.x );
	RenderColorEdit( XorStr( "Glow CT Visible" ) , XorStr( "##Colors.Visual.GlowCT_Visible" ) , &Settings::Colors::Visual::GlowCT_Visible.x );
	ImGui::EndChild();
}

auto CAndromedaMenu::RenderMenuTab() -> void
{
	RenderSectionHeader( XorStr( "System / Menu" ) , XorStr( "Telemetria de runtime e configuracoes da UI" ) );

	ImGui::BeginChild( XorStr( "##System.Menu" ) , ImVec2( 0.f , 160.f ) , true );
	RenderSliderInt( XorStr( "Menu Alpha" ) , XorStr( "##Menu.MenuAlpha" ) , Settings::Menu::MenuAlpha , 100 , 255 );

	const char* MenuStyleItems[] =
	{
		"Indigo" , "Vermillion" , "Classic Steam"
	};
	if ( RenderComboBox( XorStr( "Menu Style" ) , XorStr( "##Menu.MenuStyle" ) , Settings::Menu::MenuStyle , MenuStyleItems , IM_ARRAYSIZE( MenuStyleItems ) ) )
		GetAndromedaGUI()->UpdateStyle();
	ImGui::EndChild();

	ImGui::Spacing();
	RenderTechCard(
		XorStr( "Telemetry" ) ,
		XorStr( "AVX-512, contagem de threads e status geral da pipeline." ) ,
		XorStr( "Visibilidade imediata da capacidade de execucao atual." ) );
	ImGui::BulletText( "AVX-512 Support: %s" , CPhysicsAligner::HasAVX512() ? "YES" : "NO" );
	ImGui::BulletText( "Thread Count: %d" , CPhysicsAligner::GetOptimalThreadCount() );
}

auto CAndromedaMenu::RenderSectionHeader( const char* szTitle , const char* szSubtitle ) -> void
{
	ImGui::TextColored( ImVec4( 0.54f , 0.87f , 1.0f , 1.0f ) , szTitle );
	ImGui::TextColored( ImVec4( 0.64f , 0.69f , 0.77f , 1.0f ) , szSubtitle );
	ImGui::Separator();
}

auto CAndromedaMenu::RenderTechCard( const char* szTitle , const char* szDescription , const char* szImpact ) -> void
{
	ImGui::BeginChild( ( std::string( "##Card." ) + szTitle ).c_str() , ImVec2( 0.f , 86.f ) , true );
	ImGui::TextColored( ImVec4( 0.49f , 0.92f , 1.00f , 1.00f ) , szTitle );
	ImGui::TextWrapped( szDescription );
	ImGui::TextColored( ImVec4( 0.44f , 0.95f , 0.68f , 1.00f ) , "Impacto: %s" , szImpact );
	ImGui::EndChild();
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

	const bool bClicked = ImGui::Button( szLabel , ImVec2( -1.f , 40.f ) );

	if ( bSelected )
		ImGui::PopStyleColor( 3 );

	if ( bClicked )
		m_CurrentTab = Tab;

	return bClicked;
}

auto CAndromedaMenu::RenderCheckBox( const char* szTitle , const char* szStrID , bool& SettingsItem ) -> bool
{
	if ( szTitle )
	{
		ImGui::AlignTextToFramePadding();
		ImGui::Text( szTitle );
		ImGui::SameLine( ImGui::CalcTextSize( szTitle ).x + 10.f );
	}

	const auto LeftPadding = ImGui::GetStyle().FramePadding.x;

	ImGui::Dummy( ImVec2( ImGui::GetContentRegionAvail().x - 27.f - LeftPadding , 0.f ) );
	ImGui::SameLine();

	const auto Ret = ImGui::Checkbox( szStrID , &SettingsItem );

	return Ret;
}

auto CAndromedaMenu::RenderComboBox( const char* szTitle , const char* szStrID , int& v , const char* Items[] , int ItemsCount ) -> bool
{
	if ( szTitle )
	{
		ImGui::AlignTextToFramePadding();
		ImGui::Text( szTitle );
	}

	ImGui::SameLine();

	ImGui::PushItemWidth( -1.f );
	const auto Ret = ImGui::Combo( szStrID , &v , Items , ItemsCount );
	ImGui::PopItemWidth();

	return Ret;
}

auto CAndromedaMenu::RenderColorEdit( const char* szTitle , const char* szStrID , float* Color ) -> bool
{
	if ( szTitle )
	{
		ImGui::AlignTextToFramePadding();
		ImGui::Text( szTitle );
	}

	ImGui::SameLine();

	const auto Ret = ImGui::ColorEdit4( szStrID , Color );

	return Ret;
}

auto CAndromedaMenu::RenderSliderInt( const char* szTitle , const char* szStrID , int& Value , int Min , int Max ) -> bool
{
	if ( szTitle )
	{
		ImGui::AlignTextToFramePadding();
		ImGui::Text( szTitle );
	}

	ImGui::SameLine();

	ImGui::PushItemWidth( -1.f );
	const auto Ret = ImGui::SliderInt( szStrID , &Value , Min , Max );
	ImGui::PopItemWidth();

	return Ret;
}

auto CAndromedaMenu::RenderSliderFloat( const char* szTitle , const char* szStrID , float& Value , float Min , float Max , const char* szFormat ) -> bool
{
	if ( szTitle )
	{
		ImGui::AlignTextToFramePadding();
		ImGui::Text( szTitle );
	}

	ImGui::SameLine();

	ImGui::PushItemWidth( -1.f );
	const auto Ret = ImGui::SliderFloat( szStrID , &Value , Min , Max , szFormat );
	ImGui::PopItemWidth();

	return Ret;
}

auto GetAndromedaMenu() -> CAndromedaMenu*
{
	return &g_CAndromedaMenu;
}
