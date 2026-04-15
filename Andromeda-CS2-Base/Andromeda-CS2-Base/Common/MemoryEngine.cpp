#include "MemoryEngine.hpp"

#include <vector>
#include <immintrin.h>

#define INRANGE(x,a,b) (x >= a && x <= b) 
#define getBits( x ) (INRANGE((x&(~0x20)),'A','F') ? ((x&(~0x20)) - 'A' + 0xa) : (INRANGE(x,'0','9') ? x - '0' : 0))
#define getByte( x ) (getBits(x[0]) << 4 | getBits(x[1]))

auto GetIDAPatternSize( const char* pattern ) -> size_t
{
	auto pattern_size = 0;
	auto pattern_scan = pattern;
	auto pattern_scan_end = pattern + strlen( pattern ) + 1;

	while ( pattern_scan[0] && pattern_scan[1] && pattern_scan < pattern_scan_end )
	{
		char symbol = pattern_scan[0];
		char next_symbol = pattern_scan[1];

		auto IsByte = []( char sym )
		{
			return ( sym >= '0' && sym <= '9' || sym >= 'A' && sym <= 'F' );
		};

		if ( IsByte( symbol ) && IsByte( next_symbol ) )
		{
			pattern_size++;
			pattern_scan += 3;
			continue;
		}
		else if ( symbol == '?' )
		{
			pattern_scan += 2;
			pattern_size++;
		}
		else
			break;
	}

	return pattern_size;
}

auto FindPattern( const char* szPattern , uintptr_t StartAddr , uintptr_t EndAddr , uint32_t offset )->PVOID
{
	std::vector<uint8_t> PatternBytes;
	std::vector<uint8_t> PatternMask;

	{
		auto PatternScan = szPattern;
		auto PatternScanEnd = szPattern + strlen( szPattern ) + 1;

		while ( PatternScan[0] && PatternScan[1] && PatternScan < PatternScanEnd )
		{
			const char Symbol = PatternScan[0];
			const char NextSymbol = PatternScan[1];

			const auto IsByte = []( char Sym )
			{
				return ( Sym >= '0' && Sym <= '9' ) || ( Sym >= 'A' && Sym <= 'F' );
			};

			if ( Symbol == '?' )
			{
				PatternBytes.emplace_back( 0 );
				PatternMask.emplace_back( 0x00 );
				PatternScan += 2;
				continue;
			}

			if ( IsByte( Symbol ) && IsByte( NextSymbol ) )
			{
				PatternBytes.emplace_back( getByte( PatternScan ) );
				PatternMask.emplace_back( 0xFF );
				PatternScan += 3;
				continue;
			}

			break;
		}
	}

	const auto PatternSize = PatternBytes.size();

	if ( !PatternSize || EndAddr <= StartAddr || ( EndAddr - StartAddr ) < PatternSize )
		return nullptr;

	const auto MatchScalar = [&]( uintptr_t Address , size_t StartIndex ) -> bool
	{
		for ( size_t i = StartIndex; i < PatternSize; ++i )
		{
			if ( PatternMask[i] == 0x00 )
				continue;

			if ( *reinterpret_cast<uint8_t*>( Address + i ) != PatternBytes[i] )
				return false;
		}

		return true;
	};

	const auto MatchAddress = [&]( uintptr_t Address ) -> bool
	{
		size_t Index = 0;

#if defined( __AVX512BW__ )
		const __m512i ZeroVec = _mm512_setzero_si512();

		for ( ; Index + 64 <= PatternSize; Index += 64 )
		{
			const __m512i DataVec = _mm512_loadu_si512( reinterpret_cast<const void*>( Address + Index ) );
			const __m512i PatternVec = _mm512_loadu_si512( PatternBytes.data() + Index );
			const __m512i MaskVec = _mm512_loadu_si512( PatternMask.data() + Index );
			const __m512i DiffVec = _mm512_and_si512( _mm512_xor_si512( DataVec , PatternVec ) , MaskVec );

			if ( _mm512_cmpneq_epi8_mask( DiffVec , ZeroVec ) != 0 )
				return false;
		}
#endif

#if defined( __AVX2__ )
		const __m256i ZeroVec = _mm256_setzero_si256();

		for ( ; Index + 32 <= PatternSize; Index += 32 )
		{
			const __m256i DataVec = _mm256_loadu_si256( reinterpret_cast<const __m256i*>( Address + Index ) );
			const __m256i PatternVec = _mm256_loadu_si256( reinterpret_cast<const __m256i*>( PatternBytes.data() + Index ) );
			const __m256i MaskVec = _mm256_loadu_si256( reinterpret_cast<const __m256i*>( PatternMask.data() + Index ) );
			const __m256i DiffVec = _mm256_and_si256( _mm256_xor_si256( DataVec , PatternVec ) , MaskVec );
			const __m256i EqZero = _mm256_cmpeq_epi8( DiffVec , ZeroVec );

			if ( _mm256_movemask_epi8( EqZero ) != -1 )
				return false;
		}
#endif

		return MatchScalar( Address , Index );
	};

	const auto LastStart = EndAddr - PatternSize;

	for ( auto CurrentAddr = StartAddr; CurrentAddr <= LastStart; ++CurrentAddr )
	{
		if ( MatchAddress( CurrentAddr ) )
			return reinterpret_cast<PVOID>( CurrentAddr + offset );
	}

	return nullptr;
}

auto FindPattern( const char* szModuleName , const char* szPattern , uint32_t offset )->PVOID
{
	PVOID pFoundAddress = nullptr;

GetModuleStart:;

	auto hModule = GetModuleHandleA( szModuleName );

	if ( hModule == nullptr )
	{
		Sleep( 100 );
		goto GetModuleStart;
	}

	auto pDosHeader = (PIMAGE_DOS_HEADER)hModule;

	if ( pDosHeader->e_magic == (WORD)'ZM' )
	{
		auto pNtHeader = (PIMAGE_NT_HEADERS64)( (uintptr_t)pDosHeader + pDosHeader->e_lfanew );

		if ( pNtHeader->Signature == (WORD)'EP' )
		{
			auto Start = (DWORD_PTR)hModule + pNtHeader->OptionalHeader.BaseOfCode;
			auto End = (DWORD_PTR)hModule + pNtHeader->OptionalHeader.SizeOfCode;

			pFoundAddress = FindPattern( szPattern , Start , End , offset );
		}
	}

	return pFoundAddress;
}
