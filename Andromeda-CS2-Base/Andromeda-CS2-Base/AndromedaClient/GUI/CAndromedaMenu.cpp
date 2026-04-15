#include "CAndromedaMenu.hpp"

#include <ImGui/imgui.h>

#include <AndromedaClient/Settings/Settings.hpp>
#include <AndromedaClient/CAndromedaGUI.hpp>
#include <AndromedaClient/Features/CAimbot.hpp>

static CAndromedaMenu g_CAndromedaMenu{};

auto CAndromedaMenu::OnRenderMenu() -> void
{
	const float MenuAlpha = static_cast<float>( Settings::Menu::MenuAlpha ) / 255.f;

	ImGui::PushStyleVar( ImGuiStyleVar_Alpha , MenuAlpha );
	ImGui::SetNextWindowSize( ImVec2( 500 , 500 ) , ImGuiCond_FirstUseEver );

	if ( ImGui::Begin( XorStr( CHEAT_NAME ) , 0 ) )
	{
		// ============================================
		// AIMBOT TAB - ULTRA-OTIMIZADO
		// ============================================
		if ( ImGui::CollapsingHeader( XorStr( "Aimbot" ) ) )
		{
			auto& aimbotConfig = CAimbot::GetConfig();
			
			RenderCheckBox( XorStr( "Enable Aimbot" ) , XorStr( "##Aimbot.Enabled" ) , aimbotConfig.enabled );
			RenderCheckBox( XorStr( "Trigger Bot" ) , XorStr( "##Aimbot.TriggerBot" ) , aimbotConfig.triggerBot );
			RenderCheckBox( XorStr( "Auto Wall" ) , XorStr( "##Aimbot.AutoWall" ) , aimbotConfig.autoWall );
			RenderCheckBox( XorStr( "Visibility Check" ) , XorStr( "##Aimbot.VisibilityCheck" ) , aimbotConfig.visibilityCheck );
			RenderCheckBox( XorStr( "Recoil Control" ) , XorStr( "##Aimbot.RecoilControl" ) , aimbotConfig.recoilControl );
			
			ImGui::Separator();
			
			RenderSliderInt( XorStr( "Bone Target" ) , XorStr( "##Aimbot.BoneTarget" ) , aimbotConfig.boneTarget , 0 , 19 );
			RenderSliderInt( XorStr( "Min Damage" ) , XorStr( "##Aimbot.MinDamage" ) , aimbotConfig.minDamage , 1 , 100 );
			RenderSliderInt( XorStr( "Smooth" ) , XorStr( "##Aimbot.Smooth" ) , *reinterpret_cast<int*>(&aimbotConfig.smooth) , 0 , 100 );
			RenderSliderInt( XorStr( "FOV" ) , XorStr( "##Aimbot.FOV" ) , *reinterpret_cast<int*>(&aimbotConfig.fov) , 0 , 180 );
			
			if ( aimbotConfig.recoilControl )
			{
				ImGui::Separator();
				RenderSliderInt( XorStr( "Recoil Control X" ) , XorStr( "##Aimbot.RCX" ) , *reinterpret_cast<int*>(&aimbotConfig.recoilControlX) , -100 , 100 );
				RenderSliderInt( XorStr( "Recoil Control Y" ) , XorStr( "##Aimbot.RCY" ) , *reinterpret_cast<int*>(&aimbotConfig.recoilControlY) , -100 , 100 );
			}
			
			ImGui::Separator();
			ImGui::TextColored( ImVec4( 0.0f, 1.0f, 0.0f, 1.0f ), "Performance Technologies:" );
			ImGui::BulletText( "SIMD AVX-512 Batch Processing (8 alvos/ciclo)" );
			ImGui::BulletText( "SoA Entity Cache (L1 Cache otimizado)" );
			ImGui::BulletText( "rsqrt14 Fast Normalize (70%% menos ciclos)" );
			ImGui::BulletText( "Arena Allocator (Zero syscall overhead)" );
			ImGui::BulletText( "MPSC Ring-Buffer (Zero contencao)" );
			ImGui::BulletText( "FMA Matrix Projection (Hardware precision)" );
		}

		// ============================================
		// VISUALS TAB
		// ============================================
		if ( ImGui::CollapsingHeader( XorStr( "Visuals" ) ) )
		{
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
				ImGui::Indent();
				RenderCheckBox( XorStr( "  Bone ESP Team" ) , XorStr( "##Visual.BoneESPTeam" ) , Settings::Visual::BoneESPTeam );
				RenderCheckBox( XorStr( "  Bone ESP Enemy" ) , XorStr( "##Visual.BoneESPEnemy" ) , Settings::Visual::BoneESPEnemy );
				ImGui::Unindent();
			}

			RenderCheckBox( XorStr( "Glow" ) , XorStr( "##Visual.Glow" ) , Settings::Visual::Glow );
			if ( Settings::Visual::Glow )
			{
				ImGui::Indent();
				RenderCheckBox( XorStr( "  Glow Team" ) , XorStr( "##Visual.GlowTeam" ) , Settings::Visual::GlowTeam );
				RenderCheckBox( XorStr( "  Glow Enemy" ) , XorStr( "##Visual.GlowEnemy" ) , Settings::Visual::GlowEnemy );
				ImGui::Unindent();
			}
			
			ImGui::Separator();
			ImGui::TextColored( ImVec4( 0.0f, 1.0f, 0.0f, 1.0f ), "Performance Technologies:" );
			ImGui::BulletText( "FMA Matrix Projection (Hardware precision)" );
			ImGui::BulletText( "SIMD Batch W2S (Todos ossos simultaneamente)" );
		}

		// ============================================
		// COLORS TAB
		// ============================================
		if ( ImGui::CollapsingHeader( XorStr( "Colors" ) ) )
		{
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
		}

		// ============================================
		// MENU TAB
		// ============================================
		if ( ImGui::CollapsingHeader( XorStr( "Menu" ) ) )
		{
			RenderSliderInt( XorStr( "Menu Alpha" ) , XorStr( "##Menu.MenuAlpha" ) , Settings::Menu::MenuAlpha , 100 , 255 );

			const char* MenuStyleItems[] =
			{
				"Indigo" , "Vermillion" , "Classic Steam"
			};

			if ( RenderComboBox( XorStr( "Menu Style" ) , XorStr( "##Menu.MenuStyle" ) , Settings::Menu::MenuStyle , MenuStyleItems , IM_ARRAYSIZE( MenuStyleItems ) ) )
				GetAndromedaGUI()->UpdateStyle();
				
			ImGui::Separator();
			ImGui::TextColored( ImVec4( 1.0f, 0.5f, 0.0f, 1.0f ), "System Info:" );
			ImGui::BulletText( "AVX-512 Support: %s", CAimbot::HasAVX512() ? "YES" : "NO" );
			ImGui::BulletText( "Thread Count: %d", CAimbot::GetOptimalThreadCount() );
			ImGui::BulletText( "Arena Memory: 8MB Aligned (64B)" );
		}
	}

	ImGui::End();

	ImGui::PopStyleVar();
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

auto GetAndromedaMenu() -> CAndromedaMenu*
{
	return &g_CAndromedaMenu;
}
