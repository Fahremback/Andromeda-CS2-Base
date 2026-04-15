#pragma once

#include <array>
#include <atomic>
#include <cstddef>

template<typename T , size_t Capacity>
class alignas( 64 ) CSPSCQueue final
{
	static_assert( ( Capacity & ( Capacity - 1 ) ) == 0 , "Capacity must be power-of-two" );

public:
	auto Push( const T& Item ) -> bool
	{
		const size_t Head = m_Head.load( std::memory_order_relaxed );
		const size_t NextHead = ( Head + 1 ) & ( Capacity - 1 );

		if ( NextHead == m_Tail.load( std::memory_order_acquire ) )
			return false;

		m_Buffer[Head] = Item;
		m_Head.store( NextHead , std::memory_order_release );
		return true;
	}

	auto Pop( T& Item ) -> bool
	{
		const size_t Tail = m_Tail.load( std::memory_order_relaxed );
		if ( Tail == m_Head.load( std::memory_order_acquire ) )
			return false;

		Item = m_Buffer[Tail];
		m_Tail.store( ( Tail + 1 ) & ( Capacity - 1 ) , std::memory_order_release );
		return true;
	}

	auto SizeApprox() const -> size_t
	{
		const size_t Head = m_Head.load( std::memory_order_acquire );
		const size_t Tail = m_Tail.load( std::memory_order_acquire );
		return ( Head - Tail ) & ( Capacity - 1 );
	}

private:
	alignas( 64 ) std::array<T , Capacity> m_Buffer{};
	alignas( 64 ) std::atomic_size_t m_Head = 0;
	alignas( 64 ) std::atomic_size_t m_Tail = 0;
};
