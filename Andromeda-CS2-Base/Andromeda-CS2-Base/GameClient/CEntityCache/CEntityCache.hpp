#pragma once

#include <Common/Common.hpp>

#include <array>

#include <CS2/SDK/Types/CHandle.hpp>
#include <CS2/SDK/Types/CEntityData.hpp>

struct CachedEntity_t
{
    enum Type
    {
        UNKNOWN = 0 ,
        PLAYER_CONTROLLER ,
        BASE_WEAPON ,
        PLANTED_C4 ,
        GRENADE_PROJECTILE
    };

    CHandle m_Handle = { INVALID_EHANDLE_INDEX };
    Type m_Type = UNKNOWN;

    Rect_t m_Bbox = { 0.f , 0.f , 0.f , 0.f };

    bool m_bDraw = false;
    bool m_bVisible = false;
};

class IEntityCache
{
public:
    virtual void OnAddEntity( CEntityInstance* pInst , CHandle handle ) = 0;
    virtual void OnRemoveEntity( CEntityInstance* pInst , CHandle handle ) = 0;
};

class CEntityCache final : public IEntityCache
{
public:
    using Lock_t = std::recursive_mutex;
    static constexpr size_t MAX_CACHED_ENTITIES = 4096;

public:
    virtual void OnAddEntity( CEntityInstance* pInst , CHandle handle ) override;
    virtual void OnRemoveEntity( CEntityInstance* pInst , CHandle handle ) override;

public:
    auto GetEntityType( C_BaseEntity* pBaseEntity ) -> CachedEntity_t::Type;

public:
    inline auto GetCount() const -> size_t
    {
        return m_Count;
    }

    inline auto GetHandle( size_t index ) const -> const CHandle&
    {
        return m_Handles[index];
    }

    inline auto GetType( size_t index ) const -> CachedEntity_t::Type
    {
        return m_Types[index];
    }

    inline auto GetBBox( size_t index ) const -> const Rect_t&
    {
        return m_BBoxes[index];
    }

    inline auto ShouldDraw( size_t index ) const -> bool
    {
        return m_Draw[index];
    }

    inline auto IsVisible( size_t index ) const -> bool
    {
        return m_Visible[index];
    }

    inline auto SetVisible( size_t index , bool value ) -> void
    {
        m_Visible[index] = value;
    }

    inline auto SetDraw( size_t index , bool value ) -> void
    {
        m_Draw[index] = value;
    }

    inline auto SetBBox( size_t index , const Rect_t& value ) -> void
    {
        m_BBoxes[index] = value;
    }

    inline auto GetLock() -> Lock_t&
    {
        return m_Lock;
    }

private:
    alignas( 64 ) std::array<CHandle , MAX_CACHED_ENTITIES> m_Handles{};
    alignas( 64 ) std::array<CachedEntity_t::Type , MAX_CACHED_ENTITIES> m_Types{};
    alignas( 64 ) std::array<Rect_t , MAX_CACHED_ENTITIES> m_BBoxes{};
    alignas( 64 ) std::array<bool , MAX_CACHED_ENTITIES> m_Draw{};
    alignas( 64 ) std::array<bool , MAX_CACHED_ENTITIES> m_Visible{};
    size_t m_Count = 0;
    Lock_t m_Lock;
};

auto GetEntityCache() -> CEntityCache*;
