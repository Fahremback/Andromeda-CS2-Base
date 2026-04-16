#include "DllMain.hpp"
#include "DllLauncher.hpp"

BOOL WINAPI DllMain( HINSTANCE hInstace , DWORD dwReason , LPVOID lpReserved )
{
	switch ( dwReason )
	{
		case DLL_PROCESS_ATTACH:
			DisableThreadLibraryCalls( hInstace );
			// MessageBoxA(NULL, "Andromeda Bridge Attached", "Debug", MB_OK);
			GetDllLauncher()->OnDllMain( lpReserved , hInstace );

			break;
		case DLL_PROCESS_DETACH:
			GetDllLauncher()->OnDestroy();

			break;
	}

	return TRUE;
}
