#include "Hook_UpdateInPVS.hpp"

#include <CS2/SDK/Types/CEntityData.hpp>

auto Hook_UpdateInPVS( CEntityInstance* pEntityInstance , int unk0 ) -> bool
{
	// FIX: Always returning true forced processing of invalid entities during
	// match start, causing crashes when entity list is being rebuilt.
	// Must call original to let game engine manage entity visibility properly.
	if (!pEntityInstance)
		return false;
	return UpdateInPVS_o( pEntityInstance , unk0 );
}
