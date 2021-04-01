/*
 * entitycache.cpp
 *
 *  Created on: Nov 7, 2016
 *      Author: nullifiedcat
 */

#include "common.hpp"

#include <time.h>
#include <settings/Float.hpp>
#include "soundcache.hpp"

bool IsProjectileACrit(CachedEntity *ent)
{
    if (ent->m_bGrenadeProjectile())
        return CE_BYTE(ent, netvar.Grenade_bCritical);
    return CE_BYTE(ent, netvar.Rocket_bCritical);
}
// This method of const'ing the index is weird.
CachedEntity::CachedEntity() : m_IDX(int(((unsigned) this - (unsigned) &entity_cache::array) / sizeof(CachedEntity))), hitboxes(hitbox_cache::Get(unsigned(m_IDX)))
{
#if PROXY_ENTITY != true
    m_pEntity = nullptr;
#endif
    m_fLastUpdate = 0.0f;
}

void CachedEntity::Reset()
{
    m_bAnyHitboxVisible = false;
    m_bVisCheckComplete = false;
    m_lLastSeen         = 0;
    m_lSeenTicks        = 0;
    memset(&player_info, 0, sizeof(player_info_s));
    m_vecAcceleration.Zero();
    m_vecVOrigin.Zero();
    m_vecVelocity.Zero();
    m_fLastUpdate = 0;
}

CachedEntity::~CachedEntity()
{
}

static settings::Float ve_window{ "debug.ve.window", "0" };
static settings::Boolean ve_smooth{ "debug.ve.smooth", "true" };
static settings::Int ve_averager_size{ "debug.ve.averaging", "0" };

void CachedEntity::Update()
{
    auto raw = RAW_ENT(this);

    if (!raw)
        return;
#if PROXY_ENTITY != true
    m_pEntity = g_IEntityList->GetClientEntity(idx);
    if (!m_pEntity)
    {
        return;
    }
#endif
    m_lSeenTicks = 0;
    m_lLastSeen  = 0;

    hitboxes.Update();

    m_bVisCheckComplete = false;

    if (m_Type() == EntityType::ENTITY_PLAYER)
        g_IEngine->GetPlayerInfo(m_IDX, &player_info);
}

// FIXME maybe disable this by default
static settings::Boolean fast_vischeck{ "debug.fast-vischeck", "true" };

bool CachedEntity::IsVisible()
{
    static constexpr int optimal_hitboxes[] = { hitbox_t::head, hitbox_t::foot_L, hitbox_t::hand_R, hitbox_t::spine_1 };
    static bool vischeck0, vischeck;

    PROF_SECTION(CE_IsVisible);
    if (m_bVisCheckComplete)
        return m_bAnyHitboxVisible;

    vischeck0 = IsEntityVectorVisible(this, m_vecOrigin(), true);

    if (vischeck0)
    {
        m_bAnyHitboxVisible = true;
        m_bVisCheckComplete = true;
        return true;
    }

    if (m_Type() == ENTITY_PLAYER && fast_vischeck)
    {
        for (int i = 0; i < 4; i++)
        {
            if (hitboxes.VisibilityCheck(optimal_hitboxes[i]))
            {
                m_bAnyHitboxVisible = true;
                m_bVisCheckComplete = true;
                return true;
            }
        }
        m_bAnyHitboxVisible = false;
        m_bVisCheckComplete = true;
        return false;
    }

    for (int i = 0; i < hitboxes.m_nNumHitboxes; i++)
    {
        vischeck = false;
        vischeck = hitboxes.VisibilityCheck(i);
        if (vischeck)
        {
            m_bAnyHitboxVisible = true;
            m_bVisCheckComplete = true;
            return true;
        }
    }
    m_bAnyHitboxVisible = false;
    m_bVisCheckComplete = true;

    return false;
}

std::optional<Vector> CachedEntity::m_vecDormantOrigin()
{
    if (!RAW_ENT(this)->IsDormant())
        return m_vecOrigin();
    auto vec = soundcache::GetSoundLocation(this->m_IDX);
    if (vec)
        return *vec;
    return std::nullopt;
}

namespace entity_cache
{

CachedEntity array[MAX_ENTITIES]{};

void Update()
{
    max = g_IEntityList->GetHighestEntityIndex();
    if (max >= MAX_ENTITIES)
        max = MAX_ENTITIES - 1;
    for (int i = 0; i <= max; i++)
    {
        array[i].Update();
        if (CE_GOOD((&array[i])))
            array[i].hitboxes.UpdateBones();
    }
}

void Invalidate()
{
    for (auto &ent : array)
    {
        // pMuch useless line!
        // ent.m_pEntity = nullptr;
        ent.Reset();
    }
}

void Shutdown()
{
    for (auto &ent : array)
    {
        ent.Reset();
        ent.hitboxes.Reset();
    }
}

int max = 0;
} // namespace entity_cache
