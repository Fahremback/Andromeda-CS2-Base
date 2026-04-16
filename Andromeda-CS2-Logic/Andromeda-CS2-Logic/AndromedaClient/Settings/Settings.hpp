#pragma once

#include <Common/Include/IAndromedaLogic.hpp>
#include <AndromedaClient/CAndromedaClient.hpp>

// Macros for direct access to SharedState members
#define Active_Setting (GetAndromedaClient()->GetSettings()->Visual.Active)
#define Team_Setting (GetAndromedaClient()->GetSettings()->Visual.Team)
#define Enemy_Setting (GetAndromedaClient()->GetSettings()->Visual.Enemy)
#define OnlyVisible_Setting (GetAndromedaClient()->GetSettings()->Visual.OnlyVisible)
#define PlayerBox_Setting (GetAndromedaClient()->GetSettings()->Visual.PlayerBox)
#define BoneESP_Setting (GetAndromedaClient()->GetSettings()->Visual.BoneESP)
#define BoneESPTeam_Setting (GetAndromedaClient()->GetSettings()->Visual.BoneESPTeam)
#define BoneESPEnemy_Setting (GetAndromedaClient()->GetSettings()->Visual.BoneESPEnemy)
#define Glow_Setting (GetAndromedaClient()->GetSettings()->Visual.Glow)
#define GlowTeam_Setting (GetAndromedaClient()->GetSettings()->Visual.GlowTeam)
#define GlowEnemy_Setting (GetAndromedaClient()->GetSettings()->Visual.GlowEnemy)
#define PlayerBoxType_Setting (GetAndromedaClient()->GetSettings()->Visual.PlayerBoxType)

#define MenuAlpha_Setting (GetAndromedaClient()->GetSettings()->Menu.MenuAlpha)
#define MenuStyle_Setting (GetAndromedaClient()->GetSettings()->Menu.MenuStyle)

#define PlayerBoxTT_Setting (GetAndromedaClient()->GetSettings()->Colors.PlayerBoxTT)
#define PlayerBoxTT_Visible_Setting (GetAndromedaClient()->GetSettings()->Colors.PlayerBoxTT_Visible)
#define PlayerBoxCT_Setting (GetAndromedaClient()->GetSettings()->Colors.PlayerBoxCT)
#define PlayerBoxCT_Visible_Setting (GetAndromedaClient()->GetSettings()->Colors.PlayerBoxCT_Visible)
#define BoneESPTT_Setting (GetAndromedaClient()->GetSettings()->Colors.BoneESPTT)
#define BoneESPTT_Visible_Setting (GetAndromedaClient()->GetSettings()->Colors.BoneESPTT_Visible)
#define BoneESPCT_Setting (GetAndromedaClient()->GetSettings()->Colors.BoneESPCT)
#define BoneESPCT_Visible_Setting (GetAndromedaClient()->GetSettings()->Colors.BoneESPCT_Visible)
#define GlowTT_Setting (GetAndromedaClient()->GetSettings()->Colors.GlowTT)
#define GlowTT_Visible_Setting (GetAndromedaClient()->GetSettings()->Colors.GlowTT_Visible)
#define GlowCT_Setting (GetAndromedaClient()->GetSettings()->Colors.GlowCT)
#define GlowCT_Visible_Setting (GetAndromedaClient()->GetSettings()->Colors.GlowCT_Visible)

#define Aimbot_Enabled_Setting (GetAndromedaClient()->GetSettings()->Aimbot.Enabled)
#define Aimbot_TriggerBot_Setting (GetAndromedaClient()->GetSettings()->Aimbot.TriggerBot)
#define Aimbot_Smooth_Setting (GetAndromedaClient()->GetSettings()->Aimbot.Smooth)
#define Aimbot_FOV_Setting (GetAndromedaClient()->GetSettings()->Aimbot.FOV)
#define Aimbot_Hitchance_Setting (GetAndromedaClient()->GetSettings()->Aimbot.Hitchance)
#define Aimbot_MinDamage_Setting (GetAndromedaClient()->GetSettings()->Aimbot.MinDamage)
#define Aimbot_RCS_Setting (GetAndromedaClient()->GetSettings()->Aimbot.RCS)
#define Aimbot_RCSScaleX_Setting (GetAndromedaClient()->GetSettings()->Aimbot.RCSScaleX)
#define Aimbot_RCSScaleY_Setting (GetAndromedaClient()->GetSettings()->Aimbot.RCSScaleY)
#define Aimbot_Backtrack_Setting (GetAndromedaClient()->GetSettings()->Aimbot.Backtrack)
#define Aimbot_BacktrackTicks_Setting (GetAndromedaClient()->GetSettings()->Aimbot.BacktrackTicks)

#define Autowall_Enabled_Setting (GetAndromedaClient()->GetSettings()->Autowall.Enabled)
#define Autowall_MinDamage_Setting (GetAndromedaClient()->GetSettings()->Autowall.MinDamage)

#define AntiAim_Enabled_Setting (GetAndromedaClient()->GetSettings()->AntiAim.Enabled)
#define AntiAim_Desync_Setting (GetAndromedaClient()->GetSettings()->AntiAim.Desync)
#define AntiAim_DesyncStrength_Setting (GetAndromedaClient()->GetSettings()->AntiAim.DesyncStrength)
#define AntiAim_Pitch_Setting (GetAndromedaClient()->GetSettings()->AntiAim.Pitch)
#define AntiAim_YawJitter_Setting (GetAndromedaClient()->GetSettings()->AntiAim.YawJitter)
#define AntiAim_YawJitterAmount_Setting (GetAndromedaClient()->GetSettings()->AntiAim.YawJitterAmount)
#define AntiAim_LBYBreaker_Setting (GetAndromedaClient()->GetSettings()->AntiAim.LBYBreaker)
#define AntiAim_LBYBreakerSpeed_Setting (GetAndromedaClient()->GetSettings()->AntiAim.LBYBreakerSpeed)

#define NetworkExploits_FakeLag_Setting (GetAndromedaClient()->GetSettings()->NetworkExploits.FakeLag)
#define NetworkExploits_FakeLagTicks_Setting (GetAndromedaClient()->GetSettings()->NetworkExploits.FakeLagTicks)
#define NetworkExploits_HideShots_Setting (GetAndromedaClient()->GetSettings()->NetworkExploits.HideShots)
#define NetworkExploits_TickbaseShift_Setting (GetAndromedaClient()->GetSettings()->NetworkExploits.TickbaseShift)

#define Resolver_Enabled_Setting (GetAndromedaClient()->GetSettings()->Resolver.Enabled)
#define Resolver_BruteForce_Setting (GetAndromedaClient()->GetSettings()->Resolver.BruteForce)
#define Resolver_AnimationResolve_Setting (GetAndromedaClient()->GetSettings()->Resolver.AnimationResolve)
#define Resolver_DesyncCorrect_Setting (GetAndromedaClient()->GetSettings()->Resolver.DesyncCorrect)
#define Resolver_Extrapolation_Setting (GetAndromedaClient()->GetSettings()->Resolver.Extrapolation)
