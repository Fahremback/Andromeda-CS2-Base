#include "LinearArena.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <immintrin.h>

static CLinearArena g_LinearArena{};

namespace
{
	constexpr auto AlignUp( size_t Value , size_t Alignment ) -> size_t
	{
		return ( Value + ( Alignment - 1 ) ) & ~( Alignment - 1 );
	}
}

CLinearArena::~CLinearArena()
{
	if ( m_pBuffer )
	{
		_aligned_free( m_pBuffer );
		m_pBuffer = nullptr;
	}
}

auto CLinearArena::Init( size_t Capacity ) -> bool
{
	if ( m_pBuffer )
		return true;

	m_Capacity = AlignUp( Capacity , CACHELINE_ALIGNMENT );
	m_Offset = 0;

	m_pBuffer = reinterpret_cast<uint8_t*>( _aligned_malloc( m_Capacity , CACHELINE_ALIGNMENT ) );

	if ( !m_pBuffer )
		return false;

	Reset();
	return true;
}

auto CLinearArena::Reset() -> void
{
	if ( !m_pBuffer || !m_Capacity )
		return;

#if defined( __AVX512F__ )
	const __m512i Zero = _mm512_setzero_si512();

	for ( size_t i = 0; i < m_Capacity; i += 64 )
	{
		_mm512_store_si512( reinterpret_cast<__m512i*>( m_pBuffer + i ) , Zero );
	}
#else
	std::memset( m_pBuffer , 0 , m_Capacity );
#endif

	m_Offset = 0;
}

auto CLinearArena::Allocate( size_t Size , size_t Alignment ) -> void*
{
	if ( !m_pBuffer )
	{
		if ( !Init() )
			return nullptr;
	}

	if ( !Size )
		return nullptr;

	const auto RequestedAlignment = std::max( Alignment , CACHELINE_ALIGNMENT );
	const auto AlignedOffset = AlignUp( m_Offset , RequestedAlignment );
	if ( AlignedOffset > m_Capacity || Size > ( m_Capacity - AlignedOffset ) )
		return nullptr;
	const auto NextOffset = AlignedOffset + Size;

	auto* pOut = m_pBuffer + AlignedOffset;
	m_Offset = NextOffset;
	return pOut;
}

auto CLinearArena::PrefetchRead( const void* pMemory ) const -> void
{
	if ( pMemory )
		_mm_prefetch( reinterpret_cast<const char*>( pMemory ) , _MM_HINT_T0 );
}

auto CLinearArena::PrefetchWrite( const void* pMemory ) const -> void
{
	if ( pMemory )
		_mm_prefetch( reinterpret_cast<const char*>( pMemory ) , _MM_HINT_T0 );
}

auto GetLinearArena() -> CLinearArena*
{
	return &g_LinearArena;
}
