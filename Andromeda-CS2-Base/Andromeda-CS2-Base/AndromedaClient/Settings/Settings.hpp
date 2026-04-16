#pragma once

#include <Common/Include/IAndromedaLogic.hpp>
#include <AndromedaClient/CAndromedaClient.hpp>

namespace Settings
{
	namespace Visual
	{
		inline bool& Active() { return GetAndromedaClient()->GetSettings()->Visual.Active; }
		inline bool& Team() { return GetAndromedaClient()->GetSettings()->Visual.Team; }
		inline bool& Enemy() { return GetAndromedaClient()->GetSettings()->Visual.Enemy; }
		inline bool& OnlyVisible() { return GetAndromedaClient()->GetSettings()->Visual.OnlyVisible; }
		inline bool& PlayerBox() { return GetAndromedaClient()->GetSettings()->Visual.PlayerBox; }
		inline bool& BoneESP() { return GetAndromedaClient()->GetSettings()->Visual.BoneESP; }
		inline bool& BoneESPTeam() { return GetAndromedaClient()->GetSettings()->Visual.BoneESPTeam; }
		inline bool& BoneESPEnemy() { return GetAndromedaClient()->GetSettings()->Visual.BoneESPEnemy; }
		inline bool& Glow() { return GetAndromedaClient()->GetSettings()->Visual.Glow; }
		inline bool& GlowTeam() { return GetAndromedaClient()->GetSettings()->Visual.GlowTeam; }
		inline bool& GlowEnemy() { return GetAndromedaClient()->GetSettings()->Visual.GlowEnemy; }
		inline int& PlayerBoxType() { return GetAndromedaClient()->GetSettings()->Visual.PlayerBoxType; }
	}
	namespace Menu
	{
		inline int& MenuAlpha() { return GetAndromedaClient()->GetSettings()->Menu.MenuAlpha; }
		inline int& MenuStyle() { return GetAndromedaClient()->GetSettings()->Menu.MenuStyle; }
	}
	namespace Colors
	{
		namespace Visual
		{
			inline ImVec4& PlayerBoxTT() { return GetAndromedaClient()->GetSettings()->Colors.PlayerBoxTT; }
			inline ImVec4& PlayerBoxTT_Visible() { return GetAndromedaClient()->GetSettings()->Colors.PlayerBoxTT_Visible; }
			inline ImVec4& PlayerBoxCT() { return GetAndromedaClient()->GetSettings()->Colors.PlayerBoxCT; }
			inline ImVec4& PlayerBoxCT_Visible() { return GetAndromedaClient()->GetSettings()->Colors.PlayerBoxCT_Visible; }
			inline ImVec4& BoneESPTT() { return GetAndromedaClient()->GetSettings()->Colors.BoneESPTT; }
			inline ImVec4& BoneESPTT_Visible() { return GetAndromedaClient()->GetSettings()->Colors.BoneESPTT_Visible; }
			inline ImVec4& BoneESPCT() { return GetAndromedaClient()->GetSettings()->Colors.BoneESPCT; }
			inline ImVec4& BoneESPCT_Visible() { return GetAndromedaClient()->GetSettings()->Colors.BoneESPCT_Visible; }
			inline ImVec4& GlowTT() { return GetAndromedaClient()->GetSettings()->Colors.GlowTT; }
			inline ImVec4& GlowTT_Visible() { return GetAndromedaClient()->GetSettings()->Colors.GlowTT_Visible; }
			inline ImVec4& GlowCT() { return GetAndromedaClient()->GetSettings()->Colors.GlowCT; }
			inline ImVec4& GlowCT_Visible() { return GetAndromedaClient()->GetSettings()->Colors.GlowCT_Visible; }
		}
	}

	namespace Aimbot
	{
		inline bool& Enabled() { return GetAndromedaClient()->GetSettings()->Aimbot.Enabled; }
		inline bool& TriggerBot() { return GetAndromedaClient()->GetSettings()->Aimbot.TriggerBot; }
		inline float& Smooth() { return GetAndromedaClient()->GetSettings()->Aimbot.Smooth; }
		inline float& FOV() { return GetAndromedaClient()->GetSettings()->Aimbot.FOV; }
		inline int& Hitchance() { return GetAndromedaClient()->GetSettings()->Aimbot.Hitchance; }
		inline int& MinDamage() { return GetAndromedaClient()->GetSettings()->Aimbot.MinDamage; }
		inline bool& RCS() { return GetAndromedaClient()->GetSettings()->Aimbot.RCS; }
		inline float& RCSScaleX() { return GetAndromedaClient()->GetSettings()->Aimbot.RCSScaleX; }
		inline float& RCSScaleY() { return GetAndromedaClient()->GetSettings()->Aimbot.RCSScaleY; }
		inline bool& Backtrack() { return GetAndromedaClient()->GetSettings()->Aimbot.Backtrack; }
		inline int& BacktrackTicks() { return GetAndromedaClient()->GetSettings()->Aimbot.BacktrackTicks; }
	}

	namespace Autowall
	{
		inline bool& Enabled() { return GetAndromedaClient()->GetSettings()->Autowall.Enabled; }
		inline int& MinDamage() { return GetAndromedaClient()->GetSettings()->Autowall.MinDamage; }
	}

	namespace AntiAim
	{
		inline bool& Enabled() { return GetAndromedaClient()->GetSettings()->AntiAim.Enabled; }
		inline bool& Desync() { return GetAndromedaClient()->GetSettings()->AntiAim.Desync; }
		inline float& DesyncStrength() { return GetAndromedaClient()->GetSettings()->AntiAim.DesyncStrength; }
		inline int& Pitch() { return GetAndromedaClient()->GetSettings()->AntiAim.Pitch; }
		inline bool& YawJitter() { return GetAndromedaClient()->GetSettings()->AntiAim.YawJitter; }
		inline float& YawJitterAmount() { return GetAndromedaClient()->GetSettings()->AntiAim.YawJitterAmount; }
		inline bool& LBYBreaker() { return GetAndromedaClient()->GetSettings()->AntiAim.LBYBreaker; }
		inline float& LBYBreakerSpeed() { return GetAndromedaClient()->GetSettings()->AntiAim.LBYBreakerSpeed; }
	}

	namespace NetworkExploits
	{
		inline bool& FakeLag() { return GetAndromedaClient()->GetSettings()->NetworkExploits.FakeLag; }
		inline int& FakeLagTicks() { return GetAndromedaClient()->GetSettings()->NetworkExploits.FakeLagTicks; }
		inline bool& HideShots() { return GetAndromedaClient()->GetSettings()->NetworkExploits.HideShots; }
		inline bool& TickbaseShift() { return GetAndromedaClient()->GetSettings()->NetworkExploits.TickbaseShift; }
	}

	namespace Resolver
	{
		inline bool& Enabled() { return GetAndromedaClient()->GetSettings()->Resolver.Enabled; }
		inline bool& BruteForce() { return GetAndromedaClient()->GetSettings()->Resolver.BruteForce; }
		inline bool& AnimationResolve() { return GetAndromedaClient()->GetSettings()->Resolver.AnimationResolve; }
		inline bool& DesyncCorrect() { return GetAndromedaClient()->GetSettings()->Resolver.DesyncCorrect; }
		inline bool& Extrapolation() { return GetAndromedaClient()->GetSettings()->Resolver.Extrapolation; }
	}
}

// Global accessor aliases for compatibility with CAndromedaMenu code
#define Active_Setting (Settings::Visual::Active())
#define Team_Setting (Settings::Visual::Team())
#define Enemy_Setting (Settings::Visual::Enemy())
#define OnlyVisible_Setting (Settings::Visual::OnlyVisible())
#define PlayerBox_Setting (Settings::Visual::PlayerBox())
#define BoneESP_Setting (Settings::Visual::BoneESP())
#define BoneESPTeam_Setting (Settings::Visual::BoneESPTeam())
#define BoneESPEnemy_Setting (Settings::Visual::BoneESPEnemy())
#define Glow_Setting (Settings::Visual::Glow())
#define GlowTeam_Setting (Settings::Visual::GlowTeam())
#define GlowEnemy_Setting (Settings::Visual::GlowEnemy())
#define PlayerBoxType_Setting (Settings::Visual::PlayerBoxType())

#define MenuAlpha_Setting (Settings::Menu::MenuAlpha())
#define MenuStyle_Setting (Settings::Menu::MenuStyle())

#define PlayerBoxTT_Setting (Settings::Colors::Visual::PlayerBoxTT())
#define PlayerBoxTT_Visible_Setting (Settings::Colors::Visual::PlayerBoxTT_Visible())
#define PlayerBoxCT_Setting (Settings::Colors::Visual::PlayerBoxCT())
#define PlayerBoxCT_Visible_Setting (Settings::Colors::Visual::PlayerBoxCT_Visible())
#define BoneESPTT_Setting (Settings::Colors::Visual::BoneESPTT())
#define BoneESPTT_Visible_Setting (Settings::Colors::Visual::BoneESPTT_Visible())
#define BoneESPCT_Setting (Settings::Colors::Visual::BoneESPCT())
#define BoneESPCT_Visible_Setting (Settings::Colors::Visual::BoneESPCT_Visible())
#define GlowTT_Setting (Settings::Colors::Visual::GlowTT())
#define GlowTT_Visible_Setting (Settings::Colors::Visual::GlowTT_Visible())
#define GlowCT_Setting (Settings::Colors::Visual::GlowCT())
#define GlowCT_Visible_Setting (Settings::Colors::Visual::GlowCT_Visible())

#define Aimbot_Enabled_Setting (Settings::Aimbot::Enabled())
#define Aimbot_TriggerBot_Setting (Settings::Aimbot::TriggerBot())
#define Aimbot_Smooth_Setting (Settings::Aimbot::Smooth())
#define Aimbot_FOV_Setting (Settings::Aimbot::FOV())
#define Aimbot_Hitchance_Setting (Settings::Aimbot::Hitchance())
#define Aimbot_MinDamage_Setting (Settings::Aimbot::MinDamage())
#define Aimbot_RCS_Setting (Settings::Aimbot::RCS())
#define Aimbot_RCSScaleX_Setting (Settings::Aimbot::RCSScaleX())
#define Aimbot_RCSScaleY_Setting (Settings::Aimbot::RCSScaleY())
#define Aimbot_Backtrack_Setting (Settings::Aimbot::Backtrack())
#define Aimbot_BacktrackTicks_Setting (Settings::Aimbot::BacktrackTicks())

#define Autowall_Enabled_Setting (Settings::Autowall::Enabled())
#define Autowall_MinDamage_Setting (Settings::Autowall::MinDamage())

#define AntiAim_Enabled_Setting (Settings::AntiAim::Enabled())
#define AntiAim_Desync_Setting (Settings::AntiAim::Desync())
#define AntiAim_DesyncStrength_Setting (Settings::AntiAim::DesyncStrength())
#define AntiAim_Pitch_Setting (Settings::AntiAim::Pitch())
#define AntiAim_YawJitter_Setting (Settings::AntiAim::YawJitter())
#define AntiAim_YawJitterAmount_Setting (Settings::AntiAim::YawJitterAmount())
#define AntiAim_LBYBreaker_Setting (Settings::AntiAim::LBYBreaker())
#define AntiAim_LBYBreakerSpeed_Setting (Settings::AntiAim::LBYBreakerSpeed())

#define NetworkExploits_FakeLag_Setting (Settings::NetworkExploits::FakeLag())
#define NetworkExploits_FakeLagTicks_Setting (Settings::NetworkExploits::FakeLagTicks())
#define NetworkExploits_HideShots_Setting (Settings::NetworkExploits::HideShots())
#define NetworkExploits_TickbaseShift_Setting (Settings::NetworkExploits::TickbaseShift())

#define Resolver_Enabled_Setting (Settings::Resolver::Enabled())
#define Resolver_BruteForce_Setting (Settings::Resolver::BruteForce())
#define Resolver_AnimationResolve_Setting (Settings::Resolver::AnimationResolve())
#define Resolver_DesyncCorrect_Setting (Settings::Resolver::DesyncCorrect())
#define Resolver_Extrapolation_Setting (Settings::Resolver::Extrapolation())
