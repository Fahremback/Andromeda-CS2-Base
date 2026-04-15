#include "CEntityCache.hpp"

#include <algorithm>

static CEntityCache g_CEntityCache{};

void CEntityCache::OnAddEntity( CEntityInstance* pInst , CHandle handle )
{
	std::scoped_lock Lock( m_Lock );

	auto pBaseEntity = pInst->pEntityIdentity()->pBaseEntity();

	if ( pBaseEntity )
	{
		auto it = std::find( m_Handles.begin() , m_Handles.begin() + m_Count , handle );

		if ( it == m_Handles.begin() + m_Count )
		{
			const auto EntityType = GetEntityType( pBaseEntity );

			if ( EntityType == CachedEntity_t::UNKNOWN || m_Count >= MAX_CACHED_ENTITIES )
				return;

			m_Handles[m_Count] = handle;
			m_Types[m_Count] = EntityType;
			m_BBoxes[m_Count] = { 0.f , 0.f , 0.f , 0.f };
			m_Draw[m_Count] = false;
			m_Visible[m_Count] = false;
			++m_Count;
			return;
		}

		const auto Index = static_cast<size_t>( std::distance( m_Handles.begin() , it ) );

		m_Handles[Index] = handle;
		m_Types[Index] = GetEntityType( pBaseEntity );
	}
}

void CEntityCache::OnRemoveEntity( CEntityInstance* pInst , CHandle handle )
{
	std::scoped_lock lock( m_Lock );

	auto it = std::find( m_Handles.begin() , m_Handles.begin() + m_Count , handle );

	if ( it == m_Handles.begin() + m_Count )
		return;

	const auto Index = static_cast<size_t>( std::distance( m_Handles.begin() , it ) );
	const auto LastIndex = m_Count - 1;

	if ( Index != LastIndex )
	{
		m_Handles[Index] = m_Handles[LastIndex];
		m_Types[Index] = m_Types[LastIndex];
		m_BBoxes[Index] = m_BBoxes[LastIndex];
		m_Draw[Index] = m_Draw[LastIndex];
		m_Visible[Index] = m_Visible[LastIndex];
	}

	m_Handles[LastIndex] = { INVALID_EHANDLE_INDEX };
	m_Types[LastIndex] = CachedEntity_t::UNKNOWN;
	m_BBoxes[LastIndex] = { 0.f , 0.f , 0.f , 0.f };
	m_Draw[LastIndex] = false;
	m_Visible[LastIndex] = false;
	--m_Count;
}

auto CEntityCache::GetEntityType( C_BaseEntity* pBaseEntity ) -> CachedEntity_t::Type
{
	if ( pBaseEntity->IsBasePlayerController() )
		return CachedEntity_t::PLAYER_CONTROLLER;
	else if ( pBaseEntity->IsBasePlayerWeapon() )
		return CachedEntity_t::BASE_WEAPON;
	else if ( pBaseEntity->IsPlantedC4() )
		return CachedEntity_t::PLANTED_C4;
	else if ( pBaseEntity->IsGrenadeProjectile() )
		return CachedEntity_t::GRENADE_PROJECTILE;

	return CachedEntity_t::UNKNOWN;
}

auto GetEntityCache() -> CEntityCache*
{
	return &g_CEntityCache;
}
