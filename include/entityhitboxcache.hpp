/*
 * entityhitboxcache.hpp
 *
 *  Created on: May 25, 2017
 *      Author: nullifiedcat
 */

#pragma once

#include <cdll_int.h>
#include <studio.h>
#include <stdexcept>
#include <memory>
#include <vector>

// Forward declaration from entitycache.hpp
class CachedEntity;
constexpr int CACHE_MAX_HITBOXES = 64;

namespace hitbox_cache
{

struct CachedHitbox
{
    Vector min;
    Vector max;
    Vector center;
    mstudiobbox_t *bbox;
};

class EntityHitboxCache
{
public:
    EntityHitboxCache();
    ~EntityHitboxCache();

    CachedHitbox *GetHitbox(int id);
    void Update();
    void InvalidateCache();
    bool VisibilityCheck(int id);
    void Init();
    int GetNumHitboxes();
    void Reset();
    matrix3x4_t *GetBones(int numbones = -1);

    // for "fixing" bones to use the reconstructed ones
    void UpdateBones();

    int m_nNumHitboxes;
    bool m_bModelSet;
    bool m_bInit;
    bool m_bSuccess;

    model_t *m_pLastModel;
    CachedEntity *parent_ref; // TODO FIXME turn this into an actual reference

    bool m_VisCheckValidationFlags[CACHE_MAX_HITBOXES]{ false };
    bool m_VisCheck[CACHE_MAX_HITBOXES]{ false };
    bool m_CacheValidationFlags[CACHE_MAX_HITBOXES]{ false };
    std::vector<CachedHitbox> m_CacheInternal;

    std::vector<matrix3x4_t> bones;
    bool bones_setup{ false };
};

extern EntityHitboxCache array[2048];
inline EntityHitboxCache &Get(unsigned i)
{
    if (i > 2048)
        throw std::out_of_range("Requested out-of-range entity hitbox cache entry!");
    return array[i];
}
} // namespace hitbox_cache
