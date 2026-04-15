#include "DllLauncher.hpp"

#include <string>
#include <winternl.h>

#include <Common/CrashLog.hpp>
#include <Common/Helpers/StringHelper.hpp>
#include <Common/LinearArena.hpp>

#include <CS2/CSDK_Loader.hpp>
#include <CS2/CHook_Loader.hpp>
#include <CS2/SDK/CFunctionList.hpp>

#include <AndromedaClient/CAndromedaClient.hpp>

static CDllLauncher g_CDllLauncher{};

auto CDllLauncher::OnDllMain( LPVOID lpReserved , HINSTANCE hInstace ) -> void
{
	if ( lpReserved )
	{
		ManualMapParam_t* pParam = reinterpret_cast<ManualMapParam_t*>( lpReserved );

		if ( pParam )
		{
			m_DllDir = pParam->DllPath;
			m_DllDir += "\\";
			m_DllDir = m_DllDir.substr( 0 , m_DllDir.find_last_of( '\\' ) + 1 );
		}
	}
	else
	{
		char szDllDir[MAX_PATH];

		GetModuleFileNameA( hInstace , szDllDir , MAX_PATH );

		m_DllDir = szDllDir;
		m_DllDir = m_DllDir.substr( 0 , m_DllDir.find_last_of( '\\' ) );
		m_DllDir += '\\';
	}

	m_hDllImage = hInstace;

	m_SizeofImage = GetSizeOfImageInternal();
	m_BaseOfCode = GetBaseOfCodeInternal();

	char szGameFile[MAX_PATH] = { 0 };
	GetModuleFileNameA( 0 , szGameFile , MAX_PATH );

	m_CS2Dir = szGameFile;
	m_CS2Dir = m_CS2Dir.substr( 0 , m_CS2Dir.find_last_of( "\\/" ) );
	m_CS2Dir += '\\';

	memset( szGameFile , 0 , MAX_PATH );

	CreateThread( 0 , 0 , StartCheatTheard , lpReserved , 0 , 0 );
}

auto CDllLauncher::OnDestroy() -> void
{
	if ( !m_bDestroyed )
	{
		GetDevLog()->Destroy();
		GetHook_Loader()->DestroyHooks();
		GetCrashLog()->DestroyVectorExceptionHandler();
		
		m_bDestroyed = true;
	}
}

auto WINAPI CDllLauncher::StartCheatTheard( LPVOID lpThreadParameter ) -> DWORD
{
	// Startup performance budget targets (for future tuning):
	// - Hook bootstrap: < 5ms
	// - SDK loader: < 5ms
	// - Total startup: < 20ms
	LARGE_INTEGER PerfFreq{};
	QueryPerformanceFrequency( &PerfFreq );

	auto MeasureMs = [&PerfFreq]( auto&& Fn ) -> double
	{
		LARGE_INTEGER Start{};
		LARGE_INTEGER End{};
		QueryPerformanceCounter( &Start );
		Fn();
		QueryPerformanceCounter( &End );
		return static_cast<double>( End.QuadPart - Start.QuadPart ) * 1000.0 / static_cast<double>( PerfFreq.QuadPart );
	};

	GetDevLog()->Init();
	GetCrashLog()->InitVectorExceptionHandler();

	if ( !GetLinearArena()->Init() )
	{
		DEV_LOG( "[error] LinearArena::Init\n" );
		return 0;
	}

#if ENABLE_CONSOLE_DEBUG == 1
	DEV_LOG( "[+] StartCheatThread: %s\n" , ansi_to_utf8( GetDllDir() ).c_str() );
#endif

	auto MinHookInitOk = false;
	const auto MinHookInitMs = MeasureMs( [&]()
	{
		MinHookInitOk = GetHook_Loader()->InitalizeMH();
	} );

	if ( !MinHookInitOk )
	{
		DEV_LOG( "[error] Hook_Loader::InitalizeMH\n" );
		return 0;
	}

	DEV_LOG( "[perf] MinHook init: %.3f ms\n" , MinHookInitMs );

	auto FirstHookOk = false;
	const auto FirstHookMs = MeasureMs( [&]()
	{
		FirstHookOk = GetHook_Loader()->InstallFirstHook();
	} );

	if ( !FirstHookOk )
	{
		DEV_LOG( "[error] Hook_Loader::InstallFirstHook\n" );
		return 0;
	}

	DEV_LOG( "[perf] First hook stage: %.3f ms\n" , FirstHookMs );

	if ( !GetFunctionList()->OnInit() )
	{
		DEV_LOG( "[error] FunctionList::OnInit\n" );
		return 0;
	}

	auto SDKLoadOk = false;
	const auto SDKLoadMs = MeasureMs( [&]()
	{
		SDKLoadOk = GetSDK_Loader()->LoadSDK();
	} );

	if ( !SDKLoadOk )
	{
		DEV_LOG( "[error] CSDK_Loader::LoadSDK\n" );
		return 0;
	}

	DEV_LOG( "[perf] SDK load stage: %.3f ms\n" , SDKLoadMs );

	auto SecondHookOk = false;
	const auto SecondHookMs = MeasureMs( [&]()
	{
		SecondHookOk = GetHook_Loader()->InstallSecondHook();
	} );

	if ( !SecondHookOk )
	{
		DEV_LOG( "[error] Hook_Loader::InstallSecondHook\n" );
		return 0;
	}

	DEV_LOG( "[perf] Second hook stage: %.3f ms\n" , SecondHookMs );

	GetAndromedaClient()->OnInit();

	return 0;
}

auto GetDllDir()->std::string&
{
	return GetDllLauncher()->m_DllDir;
}

auto GetCS2Dir() -> std::string
{
	return GetDllLauncher()->m_CS2Dir;
}

auto GetDllLauncher() -> CDllLauncher*
{
	return &g_CDllLauncher;
}
