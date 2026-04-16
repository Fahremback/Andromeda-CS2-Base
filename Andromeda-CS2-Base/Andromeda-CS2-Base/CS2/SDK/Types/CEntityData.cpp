#include "CEntityData.hpp"

#include <CS2/SDK/SDK.hpp>

auto C_BaseEntity::IsBasePlayerController() -> bool
{
	return false;
}

auto C_BaseEntity::IsBasePlayerWeapon() -> bool
{
	return false;
}

auto C_BaseEntity::IsObserverPawn() -> bool
{
	return false;
}

auto C_BaseEntity::IsPlayerPawn() -> bool
{
	return false;
}

auto C_BaseEntity::IsPlantedC4() -> bool
{
	return false;
}

auto C_BaseEntity::IsC4() -> bool
{
	return false;
}
