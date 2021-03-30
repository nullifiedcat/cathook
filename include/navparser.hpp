#pragma once

#include <memory>
#include <vector>
#include "CNavFile.h"

// 1. Who the fuck put this in navparser.hpp
// 2. Why the fuck is it outside of all namespaces
// 3. There's a conflict between melee and prio_melee in navbot.cpp, so it needs to be named differently
enum Priority_list
{
    patrol = 5,
    lowprio_health,
    staynear,
    snipe_sentry,
    followbot,
    ammo,
    capture,
    engineer,
    prio_melee,
    health,
    danger,
};

namespace navparser
{
constexpr float PLAYER_WIDTH       = 49;
constexpr float HALF_PLAYER_WIDTH  = PLAYER_WIDTH / 2.0f;
constexpr float PLAYER_JUMP_HEIGHT = 72.0f;

#define TICKCOUNT_TIMESTAMP(seconds) (g_GlobalVars->tickcount + int(seconds / g_GlobalVars->interval_per_tick))

// Basic Blacklist reasons, you can add your own externally and use them
enum BlacklistReason_enum
{
    SENTRY,
    STICKY,
    ENEMY_NORMAL,
    ENEMY_DORMANT,
    BAD_BUILDING_SPOT,
    // Always last
    BLACKLIST_LENGTH
};

class BlacklistReason
{
public:
    BlacklistReason_enum value;
    int time     = 0;
    void operator=(BlacklistReason_enum const &reason)
    {
        this->value = reason;
    }
    BlacklistReason()
    {
        this->value = (BlacklistReason_enum) -1;
        this->time  = 0;
    }
    BlacklistReason(BlacklistReason_enum reason)
    {
        this->value = reason;
        this->time  = 0;
    }
    BlacklistReason(BlacklistReason_enum reason, int time)
    {
        this->value = reason;
        this->time  = time;
    }
};

struct Crumb
{
    CNavArea *navarea;
    Vector vec;
};

namespace NavEngine
{

// Is the Nav engine ready to run?
bool isReady();
// Are we currently pathing?
bool isPathing();
CNavFile *getNavFile();
// Get closest nav square to target vector
CNavArea *findClosestNavSquare(const Vector origin);
// Get the path nodes
std::vector<Crumb> *getCrumbs();
bool navTo(const Vector &destination, int priority = 5, bool should_repath = true, bool nav_to_local = true, bool is_repath = true);
// Use when something unexpected happens, e.g. vischeck fails
void abandonPath();
// Use to cancel pathing completely
void cancelPath();

// Return the whole thing
std::unordered_map<CNavArea *, BlacklistReason> *getFreeBlacklist();
// Return a specific category, we keep the same indexes to provide single element erasing
std::unordered_map<CNavArea *, BlacklistReason> getFreeBlacklist(BlacklistReason reason);

// Clear whole blacklist
void clearFreeBlacklist();
// Clear by category
void clearFreeBlacklist(BlacklistReason reason);

extern int current_priority;
} // namespace NavEngine
} // namespace navparser
