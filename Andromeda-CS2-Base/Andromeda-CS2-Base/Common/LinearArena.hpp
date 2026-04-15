#pragma once

#include <Common/Common.hpp>

#include <cstddef>
#include <cstdint>

class CLinearArena final
{
public:
	static constexpr size_t DEFAULT_CAPACITY = 8 * 1024 * 1024;
	static constexpr size_t CACHELINE_ALIGNMENT = 64;

public:
	CLinearArena() = default;
	~CLinearArena();

	CS_CLASS_NO_ASSIGNMENT( CLinearArena );

public:
	auto Init( size_t Capacity = DEFAULT_CAPACITY ) -> bool;
	auto Reset() -> void;

	auto Allocate( size_t Size , size_t Alignment = CACHELINE_ALIGNMENT ) -> void*;

	template< typename T >
	auto AllocateArray( size_t Count ) -> T*
	{
		return reinterpret_cast<T*>( Allocate( sizeof( T ) * Count , alignof( T ) > CACHELINE_ALIGNMENT ? alignof( T ) : CACHELINE_ALIGNMENT ) );
	}

	auto PrefetchRead( const void* pMemory ) const -> void;
	auto PrefetchWrite( const void* pMemory ) const -> void;

	auto GetCapacity() const -> size_t { return m_Capacity; }
	auto GetUsed() const -> size_t { return m_Offset; }

private:
	uint8_t* m_pBuffer = nullptr;
	size_t m_Capacity = 0;
	size_t m_Offset = 0;
};

auto GetLinearArena() -> CLinearArena*;
