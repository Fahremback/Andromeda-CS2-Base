#include "CInjector.h"

#include <array>
#include <string>

namespace
{
	constexpr const char* kDllName = "Andromeda-Bridge.dll";

	auto AppendPath( const char* basePath , const char* relativePath ) -> std::string
	{
		std::string fullPath = basePath ? basePath : "";
		if ( !fullPath.empty() && fullPath.back() != '\\' && fullPath.back() != '/' )
			fullPath += '\\';
		fullPath += relativePath;
		return fullPath;
	}
}

auto CInjector::Init() -> bool
{
	GetCurrentDirectoryA( MAX_PATH , szCurrentDir );

	if ( GetPrivileges() && ResolveDllPath() )
		return true;

	return false;
}

auto CInjector::ResolveDllPath() -> bool
{
	const std::array<std::string , 6> candidatePaths = {
		AppendPath( szCurrentDir , kDllName ) ,
		AppendPath( szCurrentDir , "x64\\Release\\Andromeda-Bridge.dll" ) ,
		AppendPath( szCurrentDir , "Andromeda-CS2-Base\\x64\\Release\\Andromeda-Bridge.dll" ) ,
		AppendPath( szCurrentDir , "..\\Andromeda-CS2-Base\\x64\\Release\\Andromeda-Bridge.dll" ) ,
		AppendPath( szCurrentDir , "..\\x64\\Release\\Andromeda-Bridge.dll" ) ,
		AppendPath( szCurrentDir , "..\\..\\Andromeda-CS2-Base\\x64\\Release\\Andromeda-Bridge.dll" )
	};

	for ( const auto& candidate : candidatePaths )
	{
		if ( !FileExist( candidate.c_str() ) )
			continue;

		strncpy_s( szDllFilePath , candidate.c_str() , _TRUNCATE );
		DEV_LOG( "[info] DLL encontrada: %s\n" , szDllFilePath );
		return true;
	}

	DEV_LOG( "[error] DLL nao encontrada. Caminhos testados:\n" );
	for ( const auto& candidate : candidatePaths )
	{
		DEV_LOG( "  - %s\n" , candidate.c_str() );
	}

	return false;
}

auto CInjector::GetPrivileges() -> bool
{
	HANDLE hToken = NULL;
	LUID luid;
	TOKEN_PRIVILEGES tp;

	OpenProcessToken( GetCurrentProcess() , TOKEN_ALL_ACCESS , &hToken );

	LookupPrivilegeValue( NULL , SE_DEBUG_NAME , &luid );

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	if ( AdjustTokenPrivileges( hToken , FALSE , &tp , sizeof( TOKEN_PRIVILEGES ) , NULL , NULL ) )
	{
		CloseHandle( hToken );
		return true;
	}

	return false;
}

auto CInjector::GetProcessIdByName( const char* szProcName )->DWORD
{
	HANDLE hSnapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS , 0 );
	DWORD dwGetProcessID = 0;

	if ( hSnapshot != INVALID_HANDLE_VALUE )
	{
		PROCESSENTRY32 ProcEntry32 = { 0 };
		ProcEntry32.dwSize = sizeof( PROCESSENTRY32 );

		if ( Process32First( hSnapshot , &ProcEntry32 ) )
		{
			do
			{
				if ( _stricmp( ProcEntry32.szExeFile , szProcName ) == 0 )
				{
					dwGetProcessID = (DWORD)ProcEntry32.th32ProcessID;
					break;
				}
			} while ( Process32Next( hSnapshot , &ProcEntry32 ) );
		}

		CloseHandle( hSnapshot );
	}

	return dwGetProcessID;
}

auto CInjector::InjectManualMap( const char* szProcessName ) -> bool
{
	bool Result = false;
	DWORD PID = 0;

	DEV_LOG( "[info] Wait for start %s\n" , szProcessName );

	uint32_t waitIterations = 0;
	while ( !PID )
	{
		PID = GetProcessIdByName( szProcessName );
		if ( PID )
			break;

		if ( ++waitIterations >= 200 )
		{
			DEV_LOG( "[-] inject code: timeout waiting process\n" );
			return false;
		}

		Sleep( 100 );
	}

	DEV_LOG( "[info] Found process %s pid=%lu\n" , szProcessName , PID );

	m_hProcess = OpenProcess( PROCESS_ALL_ACCESS , FALSE , PID );

	if ( !m_hProcess )
	{
		DEV_LOG( "[-] inject code: #1\n" );
		return false;
	}

	// Read Dll File
	{
		auto hFile = CreateFileA( szDllFilePath , GENERIC_READ , 0 , NULL , OPEN_EXISTING , FILE_ATTRIBUTE_NORMAL , NULL );

		if ( !hFile )
		{
			DEV_LOG( "[-] inject code: #2\n" );
			CloseHandle( m_hProcess );
			return false;
		}

		auto dwLength = GetFileSize( hFile , NULL );

		if ( dwLength == INVALID_FILE_SIZE || dwLength == 0 )
		{
			DEV_LOG( "[-] inject code: #3\n" );

			CloseHandle( hFile );
			CloseHandle( m_hProcess );

			return false;
		}

		m_pDllFile = (PBYTE)HeapAlloc( GetProcessHeap() , 0 , dwLength );

		if ( !m_pDllFile )
		{
			DEV_LOG( "[-] inject code: #4\n" );

			CloseHandle( hFile );
			CloseHandle( m_hProcess );

			return false;
		}

		if ( ReadFile( hFile , m_pDllFile , dwLength , &m_DllFileSize , NULL ) == FALSE )
		{
			DEV_LOG( "[-] inject code: #5\n" );

			CloseHandle( hFile );
			CloseHandle( m_hProcess );

			return false;
		}

		if ( dwLength != m_DllFileSize )
		{
			DEV_LOG( "[-] inject code: #6\n" );

			CloseHandle( hFile );
			CloseHandle( m_hProcess );

			return false;
		}

		CloseHandle( hFile );
	}

	DllLoaderData_t LoaderData = { 0 };

	// Loader Data
	memcpy( LoaderData.DllPath , szCurrentDir , MAX_PATH );

	// Inject To Process
	{
		const size_t dllPathLength = lstrlenA( szDllFilePath ) + 1;
		LPVOID pRemoteDllPath = VirtualAllocEx( m_hProcess , nullptr , dllPathLength , MEM_RESERVE | MEM_COMMIT , PAGE_READWRITE );

		if ( !pRemoteDllPath )
		{
			DEV_LOG( "[-] inject code: #7\n" );
		}
		else if ( !WriteProcessMemory( m_hProcess , pRemoteDllPath , szDllFilePath , dllPathLength , nullptr ) )
		{
			DEV_LOG( "[-] inject code: #8\n" );
			VirtualFreeEx( m_hProcess , pRemoteDllPath , 0 , MEM_RELEASE );
		}
		else
		{
			const HMODULE hKernel32 = GetModuleHandleA( "kernel32.dll" );
			auto pLoadLibraryA = reinterpret_cast<LPTHREAD_START_ROUTINE>( GetProcAddress( hKernel32 , "LoadLibraryA" ) );

			if ( !pLoadLibraryA )
			{
				DEV_LOG( "[-] inject code: #9\n" );
				VirtualFreeEx( m_hProcess , pRemoteDllPath , 0 , MEM_RELEASE );
			}
			else
			{
				HANDLE hRemoteThread = CreateRemoteThread( m_hProcess , nullptr , 0 , pLoadLibraryA , pRemoteDllPath , 0 , nullptr );
				if ( !hRemoteThread )
				{
					DEV_LOG( "[-] inject code: #10\n" );
					VirtualFreeEx( m_hProcess , pRemoteDllPath , 0 , MEM_RELEASE );
				}
				else
				{
					const DWORD waitResult = WaitForSingleObject( hRemoteThread , 10000 );

					DWORD remoteExitCode = 0;
					if ( waitResult == WAIT_OBJECT_0 && GetExitCodeThread( hRemoteThread , &remoteExitCode ) && remoteExitCode != 0 )
					{
						Result = true;
					}
					else if ( waitResult == WAIT_TIMEOUT )
					{
						DEV_LOG( "[-] inject code: #11 timeout\n" );
					}
					else
					{
						DEV_LOG( "[-] inject code: #11 exit=%lu wait=%lu\n" , remoteExitCode , waitResult );
					}

					CloseHandle( hRemoteThread );
					VirtualFreeEx( m_hProcess , pRemoteDllPath , 0 , MEM_RELEASE );
				}
			}
		}
	}

	// Free And Close
	{
		if ( m_pDllFile )
			HeapFree( GetProcessHeap() , 0 , m_pDllFile );

		CloseHandle( m_hProcess );
	}

	return Result;
}

auto GetInjector() -> CInjector*
{
	return CInjector::Instance();
}
