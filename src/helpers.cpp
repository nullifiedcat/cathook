/*
 * helpers.cpp
 *
 *  Created on: Oct 8, 2016
 *      Author: nullifiedcat
 */

#include "common.hpp"

#include <sys/mman.h>
#include "settings/Bool.hpp"
#include "MiscTemporary.hpp"
#include "PlayerTools.hpp"
#include "Ragdolls.hpp"
#include "WeaponData.hpp"

static settings::Boolean tcm{ "debug.tcm", "true" };
static settings::Boolean should_correct_punch{ "debug.correct-punch", "true" };

std::vector<ConVar *> &RegisteredVarsList()
{
    static std::vector<ConVar *> list{};
    return list;
}

std::vector<ConCommand *> &RegisteredCommandsList()
{
    static std::vector<ConCommand *> list{};
    return list;
}

void BeginConVars()
{
    logging::Info("Begin ConVars");
    if (!std::ifstream("tf/cfg/cat_autoexec_textmode.cfg"))
    {
        std::ofstream cfg_autoexec_textmode("tf/cfg/cat_autoexec_textmode.cfg", std::ios::out | std::ios::trunc);
        if (cfg_autoexec_textmode.good())
        {
            cfg_autoexec_textmode << "// Put your custom cathook settings in this "
                                     "file\n// This script will be executed EACH TIME "
                                     "YOU INJECT TEXTMODE CATHOOK\n"
                                     "\n"
                                     "engine_no_focus_sleep 0\n"
                                     "hud_fastswitch 1\n"
                                     "tf_medigun_autoheal 1\n"
                                     "fps_max 67\n"
                                     "cat_ipc_connect";
        }
    }
    if (!std::ifstream("tf/cfg/cat_autoexec.cfg"))
    {
        std::ofstream cfg_autoexec("tf/cfg/cat_autoexec.cfg", std::ios::out | std::ios::trunc);
        if (cfg_autoexec.good())
        {
            cfg_autoexec << "// Put your custom cathook settings in this "
                            "file\n// This script will be executed EACH TIME "
                            "YOU INJECT CATHOOK\n";
        }
    }
    if (!std::ifstream("tf/cfg/cat_matchexec.cfg"))
    {
        std::ofstream cfg_autoexec("tf/cfg/cat_matchexec.cfg", std::ios::out | std::ios::trunc);
        if (cfg_autoexec.good())
        {
            cfg_autoexec << "// Put your custom cathook settings in this "
                            "file\n// This script will be executed EACH TIME "
                            "YOU JOIN A MATCH\n";
        }
    }
    logging::Info(":b:");
    SetCVarInterface(g_ICvar);
}

void EndConVars()
{
    logging::Info("Registering CatCommands");
    RegisterCatCommands();
    ConVar_Register();

    std::ofstream cfg_defaults("tf/cfg/cat_defaults.cfg", std::ios::out | std::ios::trunc);
    if (cfg_defaults.good())
    {
        cfg_defaults << "// This file is auto-generated and will be "
                        "overwritten each time you inject cathook\n// Do not "
                        "make edits to this file\n\n// Every registered "
                        "variable dump\n";
        for (const auto &i : RegisteredVarsList())
        {
            cfg_defaults << i->GetName() << " \"" << i->GetDefault() << "\"\n";
        }
        cfg_defaults << "\n// Every registered command dump\n";
        for (const auto &i : RegisteredCommandsList())
        {
            cfg_defaults << "// " << i->GetName() << "\n";
        }
    }
}

ConVar *CreateConVar(std::string name, std::string value, std::string help)
{
    char *namec  = new char[256];
    char *valuec = new char[256];
    char *helpc  = new char[256];
    strncpy(namec, name.c_str(), 255);
    strncpy(valuec, value.c_str(), 255);
    strncpy(helpc, help.c_str(), 255);
    // logging::Info("Creating ConVar: %s %s %s", namec, valuec, helpc);
    ConVar *ret = new ConVar(const_cast<const char *>(namec), const_cast<const char *>(valuec), 0, const_cast<const char *>(helpc));
    g_ICvar->RegisterConCommand(ret);
    RegisteredVarsList().push_back(ret);
    return ret;
}

// Function for when you want to goto a vector
void WalkTo(const Vector &vector)
{
    if (CE_BAD(LOCAL_E))
        return;
    // Calculate how to get to a vector
    auto result = ComputeMove(LOCAL_E->m_vecOrigin(), vector);
    // Push our move to usercmd
    current_user_cmd->forwardmove = result.x;
    current_user_cmd->sidemove    = result.y;
    current_user_cmd->upmove      = result.z;
}

// Function to get the corner location that a vischeck to an entity is possible
// from
Vector VischeckCorner(CachedEntity *player, CachedEntity *target, float maxdist, bool checkWalkable)
{
    int maxiterations = maxdist / 40;
    Vector origin     = player->m_vecOrigin();

    // if we can see an entity, we don't need to run calculations
    if (VisCheckEntFromEnt(player, target))
    {
        if (!checkWalkable)
            return origin;
        else if (canReachVector(origin, target->m_vecOrigin()))
            return origin;
    }

    for (int i = 0; i < 8; i++) // for loop for all 4 directions
    {
        // 40 * maxiterations = range in HU
        for (int j = 0; j < maxiterations; j++)
        {
            Vector virtualOrigin = origin;
            // what direction to go in
            switch (i)
            {
            case 0:
                virtualOrigin.x = virtualOrigin.x + 40 * (j + 1);
                break;
            case 1:
                virtualOrigin.x = virtualOrigin.x - 40 * (j + 1);
                break;
            case 2:
                virtualOrigin.y = virtualOrigin.y + 40 * (j + 1);
                break;
            case 3:
                virtualOrigin.y = virtualOrigin.y - 40 * (j + 1);
                break;
            case 4:
                virtualOrigin.x = virtualOrigin.x + 20 * (j + 1);
                virtualOrigin.y = virtualOrigin.y + 20 * (j + 1);
                break;
            case 5:
                virtualOrigin.x = virtualOrigin.x - 20 * (j + 1);
                virtualOrigin.y = virtualOrigin.y - 20 * (j + 1);
                break;
            case 6:
                virtualOrigin.x = virtualOrigin.x - 20 * (j + 1);
                virtualOrigin.y = virtualOrigin.y + 20 * (j + 1);
                break;
            case 7:
                virtualOrigin.x = virtualOrigin.x + 20 * (j + 1);
                virtualOrigin.y = virtualOrigin.y - 20 * (j + 1);
                break;
            }
            // check if player can see the players virtualOrigin
            if (!IsVectorVisible(origin, virtualOrigin, true))
                continue;
            // check if the virtualOrigin can see the target
            if (!VisCheckEntFromEntVector(virtualOrigin, player, target))
                continue;
            if (!checkWalkable)
                return virtualOrigin;

            // check if the location is accessible
            if (!canReachVector(origin, virtualOrigin))
                continue;
            if (canReachVector(virtualOrigin, target->m_vecOrigin()))
                return virtualOrigin;
        }
    }
    // if we didn't find anything, return an empty Vector
    return { 0, 0, 0 };
}

// return Two Corners that connect perfectly to ent and local player
std::pair<Vector, Vector> VischeckWall(CachedEntity *player, CachedEntity *target, float maxdist, bool checkWalkable)
{
    int maxiterations = maxdist / 40;
    Vector origin     = player->m_vecOrigin();

    // if we can see an entity, we don't need to run calculations
    if (VisCheckEntFromEnt(player, target))
    {
        std::pair<Vector, Vector> orig(origin, target->m_vecOrigin());
        if (!checkWalkable)
            return orig;
        else if (canReachVector(origin, target->m_vecOrigin()))
            return orig;
    }

    for (int i = 0; i < 8; i++) // for loop for all 4 directions
    {
        // 40 * maxiterations = range in HU
        for (int j = 0; j < maxiterations; j++)
        {
            Vector virtualOrigin = origin;
            // what direction to go in
            switch (i)
            {
            case 0:
                virtualOrigin.x = virtualOrigin.x + 40 * (j + 1);
                break;
            case 1:
                virtualOrigin.x = virtualOrigin.x - 40 * (j + 1);
                break;
            case 2:
                virtualOrigin.y = virtualOrigin.y + 40 * (j + 1);
                break;
            case 3:
                virtualOrigin.y = virtualOrigin.y - 40 * (j + 1);
                break;
            case 4:
                virtualOrigin.x = virtualOrigin.x + 20 * (j + 1);
                virtualOrigin.y = virtualOrigin.y + 20 * (j + 1);
                break;
            case 5:
                virtualOrigin.x = virtualOrigin.x - 20 * (j + 1);
                virtualOrigin.y = virtualOrigin.y - 20 * (j + 1);
                break;
            case 6:
                virtualOrigin.x = virtualOrigin.x - 20 * (j + 1);
                virtualOrigin.y = virtualOrigin.y + 20 * (j + 1);
                break;
            case 7:
                virtualOrigin.x = virtualOrigin.x + 20 * (j + 1);
                virtualOrigin.y = virtualOrigin.y - 20 * (j + 1);
                break;
            }
            // check if player can see the players virtualOrigin
            if (!IsVectorVisible(origin, virtualOrigin, true))
                continue;
            for (int i = 0; i < 8; i++) // for loop for all 4 directions
            {
                // 40 * maxiterations = range in HU
                for (int j = 0; j < maxiterations; j++)
                {
                    Vector virtualOrigin2 = target->m_vecOrigin();
                    // what direction to go in
                    switch (i)
                    {
                    case 0:
                        virtualOrigin2.x = virtualOrigin2.x + 40 * (j + 1);
                        break;
                    case 1:
                        virtualOrigin2.x = virtualOrigin2.x - 40 * (j + 1);
                        break;
                    case 2:
                        virtualOrigin2.y = virtualOrigin2.y + 40 * (j + 1);
                        break;
                    case 3:
                        virtualOrigin2.y = virtualOrigin2.y - 40 * (j + 1);
                        break;
                    case 4:
                        virtualOrigin2.x = virtualOrigin2.x + 20 * (j + 1);
                        virtualOrigin2.y = virtualOrigin2.y + 20 * (j + 1);
                        break;
                    case 5:
                        virtualOrigin2.x = virtualOrigin2.x - 20 * (j + 1);
                        virtualOrigin2.y = virtualOrigin2.y - 20 * (j + 1);
                        break;
                    case 6:
                        virtualOrigin2.x = virtualOrigin2.x - 20 * (j + 1);
                        virtualOrigin2.y = virtualOrigin2.y + 20 * (j + 1);
                        break;
                    case 7:
                        virtualOrigin2.x = virtualOrigin2.x + 20 * (j + 1);
                        virtualOrigin2.y = virtualOrigin2.y - 20 * (j + 1);
                        break;
                    }
                    // check if the virtualOrigin2 can see the target
                    //                    if
                    //                    (!VisCheckEntFromEntVector(virtualOrigin2,
                    //                    player, target))
                    //                        continue;
                    //                    if (!IsVectorVisible(virtualOrigin,
                    //                    virtualOrigin2, true))
                    //                        continue;
                    //                    if (!IsVectorVisible(virtualOrigin2,
                    //                    target->m_vecOrigin(), true))
                    //                    	continue;
                    if (!IsVectorVisible(virtualOrigin, virtualOrigin2, true))
                        continue;
                    if (!IsVectorVisible(virtualOrigin2, target->m_vecOrigin()))
                        continue;
                    std::pair<Vector, Vector> toret(virtualOrigin, virtualOrigin2);
                    if (!checkWalkable)
                        return toret;
                    // check if the location is accessible
                    if (!canReachVector(origin, virtualOrigin) || !canReachVector(virtualOrigin2, virtualOrigin))
                        continue;
                    if (canReachVector(virtualOrigin2, target->m_vecOrigin()))
                        return toret;
                }
            }
        }
    }
    // if we didn't find anything, return an empty Vector
    return { { 0, 0, 0 }, { 0, 0, 0 } };
}

// Returns a vectors max value. For example: {123,-150, 125} = 125
float vectorMax(Vector i)
{
    return fmaxf(fmaxf(i.x, i.y), i.z);
}

// Returns a vectors absolute value. For example {123,-150, 125} = {123,150,
// 125}
Vector vectorAbs(Vector i)
{
    Vector result = i;
    result.x      = fabsf(result.x);
    result.y      = fabsf(result.y);
    result.z      = fabsf(result.z);
    return result;
}

// check to see if we can reach a vector or if it is too high / doesn't leave
// enough space for the player, optional second vector
bool canReachVector(Vector loc, Vector dest)
{
    if (!dest.IsZero())
    {
        Vector dist       = dest - loc;
        int maxiterations = floor(dest.DistTo(loc)) / 40;
        for (int i = 0; i < maxiterations; i++)
        {
            // math to get the next vector 40.0f in the direction of dest
            Vector vec = loc + dist / vectorMax(vectorAbs(dist)) * 40.0f * (i + 1);

            if (DistanceToGround({ vec.x, vec.y, vec.z + 5 }) >= 40)
                return false;

            for (int j = 0; j < 4; j++)
            {
                Vector directionalLoc = vec;
                // what direction to check
                switch (j)
                {
                case 0:
                    directionalLoc.x = directionalLoc.x + 40;
                    break;
                case 1:
                    directionalLoc.x = directionalLoc.x - 40;
                    break;
                case 2:
                    directionalLoc.y = directionalLoc.y + 40;
                    break;
                case 3:
                    directionalLoc.y = directionalLoc.y - 40;
                    break;
                }
                trace_t trace;
                Ray_t ray;
                ray.Init(vec, directionalLoc);
                {
                    PROF_SECTION(IEVV_TraceRay);
                    g_ITrace->TraceRay(ray, 0x4200400B, &trace::filter_no_player, &trace);
                }
                // distance of trace < than 26
                if (trace.startpos.DistTo(trace.endpos) < 26.0f)
                    return false;
            }
        }
    }
    else
    {
        // check if the vector is too high above ground
        // higher to avoid small false positives, player can jump 42 hu
        // according to
        // the tf2 wiki
        if (DistanceToGround({ loc.x, loc.y, loc.z + 5 }) >= 40)
            return false;

        // check if there is enough space arround the vector for a player to fit
        // for loop for all 4 directions
        for (int i = 0; i < 4; i++)
        {
            Vector directionalLoc = loc;
            // what direction to check
            switch (i)
            {
            case 0:
                directionalLoc.x = directionalLoc.x + 40;
                break;
            case 1:
                directionalLoc.x = directionalLoc.x - 40;
                break;
            case 2:
                directionalLoc.y = directionalLoc.y + 40;
                break;
            case 3:
                directionalLoc.y = directionalLoc.y - 40;
                break;
            }
            trace_t trace;
            Ray_t ray;
            ray.Init(loc, directionalLoc);
            {
                PROF_SECTION(IEVV_TraceRay);
                g_ITrace->TraceRay(ray, 0x4200400B, &trace::filter_no_player, &trace);
            }
            // distance of trace < than 26
            if (trace.startpos.DistTo(trace.endpos) < 26.0f)
                return false;
        }
    }
    return true;
}

std::string GetLevelName()
{
    std::string name(g_IEngine->GetLevelName());
    size_t slash = name.find('/');
    if (slash == std::string::npos)
        slash = 0;
    else
        slash++;
    size_t bsp    = name.find(".bsp");
    size_t length = (bsp == std::string::npos ? name.length() - slash : bsp - slash);
    return name.substr(slash, length);
}

std::pair<float, float> ComputeMovePrecise(const Vector &a, const Vector &b)
{
    Vector diff = (b - a);
    if (diff.Length() == 0.0f)
        return { 0, 0 };
    const float x = diff.x;
    const float y = diff.y;
    Vector vsilent(x, y, 0);
    Vector ang;
    VectorAngles(vsilent, ang);
    float yaw = DEG2RAD(ang.y - current_user_cmd->viewangles.y);
    if (g_pLocalPlayer->bUseSilentAngles)
        yaw = DEG2RAD(ang.y - g_pLocalPlayer->v_OrigViewangles.y);
    float speed = MAX(MIN(diff.Length() * 33.0f, 450.0f), 1.0001f);
    return { cos(yaw) * speed, -sin(yaw) * speed };
}

Vector ComputeMove(const Vector &a, const Vector &b)
{
    Vector diff = (b - a);
    if (diff.Length() == 0.0f)
        return Vector(0.0f);
    const float x = diff.x;
    const float y = diff.y;
    Vector vsilent(x, y, 0);
    Vector ang;
    VectorAngles(vsilent, ang);
    float yaw   = DEG2RAD(ang.y - current_user_cmd->viewangles.y);
    float pitch = DEG2RAD(ang.x - current_user_cmd->viewangles.x);
    if (g_pLocalPlayer->bUseSilentAngles)
    {
        yaw   = DEG2RAD(ang.y - g_pLocalPlayer->v_OrigViewangles.y);
        pitch = DEG2RAD(ang.x - g_pLocalPlayer->v_OrigViewangles.x);
    }
    Vector move = { cos(yaw) * 450.0f, -sin(yaw) * 450.0f, -cos(pitch) * 450.0f };

    // Only apply upmove in water
    if (!(g_ITrace->GetPointContents(g_pLocalPlayer->v_Eye) & CONTENTS_WATER))
        move.z = current_user_cmd->upmove;
    return move;
}

ConCommand *CreateConCommand(const char *name, FnCommandCallback_t callback, const char *help)
{
    ConCommand *ret = new ConCommand(name, callback, help);
    g_ICvar->RegisterConCommand(ret);
    RegisteredCommandsList().push_back(ret);
    return ret;
}

const char *GetBuildingName(CachedEntity *ent)
{
    if (!ent)
        return "[NULL]";
    int classid = ent->m_iClassID();
    if (classid == CL_CLASS(CObjectSentrygun))
        return "Sentry";
    if (classid == CL_CLASS(CObjectDispenser))
        return "Dispenser";
    if (classid == CL_CLASS(CObjectTeleporter))
        return "Teleporter";
    return "[NULL]";
}

void format_internal(std::stringstream &stream)
{
    (void) (stream);
}

void ReplaceString(std::string &input, const std::string &what, const std::string &with_what)
{
    size_t index;
    index = input.find(what);
    while (index != std::string::npos)
    {
        input.replace(index, what.size(), with_what);
        index = input.find(what, index + with_what.size());
    }
}

void ReplaceSpecials(std::string &str)
{
    int val;
    size_t c = 0, len = str.size();
    for (int i = 0; i + c < len; ++i)
    {
        str[i] = str[i + c];
        if (str[i] != '\\')
            continue;
        if (i + c + 1 == len)
            break;
        switch (str[i + c + 1])
        {
        // Several control characters
        case 'b':
            ++c;
            str[i] = '\b';
            break;
        case 'n':
            ++c;
            str[i] = '\n';
            break;
        case 'v':
            ++c;
            str[i] = '\v';
            break;
        case 'r':
            ++c;
            str[i] = '\r';
            break;
        case 't':
            ++c;
            str[i] = '\t';
            break;
        case 'f':
            ++c;
            str[i] = '\f';
            break;
        case 'a':
            ++c;
            str[i] = '\a';
            break;
        case 'e':
            ++c;
            str[i] = '\e';
            break;
        // Write escaped escape character as is
        case '\\':
            ++c;
            break;
        // Convert specified value from HEX
        case 'x':
            if (i + c + 4 > len)
                continue;
            std::sscanf(&str[i + c + 2], "%02X", &val);
            c += 3;
            str[i] = val;
            break;
        // Convert from unicode
        case 'u':
            if (i + c + 6 > len)
                continue;
            // 1. Scan 16bit HEX value
            std::sscanf(&str[i + c + 2], "%04X", &val);
            c += 5;
            // 2. Convert value to UTF-8
            if (val <= 0x7F)
            {
                str[i] = val;
            }
            else if (val <= 0x7FF)
            {
                str[i]     = 0xC0 | ((val >> 6) & 0x1F);
                str[i + 1] = 0x80 | (val & 0x3F);
                ++i;
                --c;
            }
            else
            {
                str[i]     = 0xE0 | ((val >> 12) & 0xF);
                str[i + 1] = 0x80 | ((val >> 6) & 0x3F);
                str[i + 2] = 0x80 | (val & 0x3F);
                i += 2;
                c -= 2;
            }
            break;
        }
    }
    str.resize(len - c);
}

powerup_type GetPowerupOnPlayer(CachedEntity *player)
{
    if (CE_BAD(player))
        return powerup_type::not_powerup;
    //	if (!HasCondition<TFCond_HasRune>(player)) return
    // powerup_type::not_powerup;
    if (HasCondition<TFCond_RuneStrength>(player))
        return powerup_type::strength;
    if (HasCondition<TFCond_RuneHaste>(player))
        return powerup_type::haste;
    if (HasCondition<TFCond_RuneRegen>(player))
        return powerup_type::regeneration;
    if (HasCondition<TFCond_RuneResist>(player))
        return powerup_type::resistance;
    if (HasCondition<TFCond_RuneVampire>(player))
        return powerup_type::vampire;
    if (HasCondition<TFCond_RuneWarlock>(player))
        return powerup_type::reflect;
    if (HasCondition<TFCond_RunePrecision>(player))
        return powerup_type::precision;
    if (HasCondition<TFCond_RuneAgility>(player))
        return powerup_type::agility;
    if (HasCondition<TFCond_RuneKnockout>(player))
        return powerup_type::knockout;
    if (HasCondition<TFCond_KingRune>(player))
        return powerup_type::king;
    if (HasCondition<TFCond_PlagueRune>(player))
        return powerup_type::plague;
    if (HasCondition<TFCond_SupernovaRune>(player))
        return powerup_type::supernova;
    return powerup_type::not_powerup;
}
// A function to find a weapon by WeaponID
int getWeaponByID(CachedEntity *player, int weaponid)
{
    // Invalid player
    if (CE_BAD(player))
        return -1;
    int *hWeapons = &CE_INT(player, netvar.hMyWeapons);
    // Go through the handle array and search for the item
    for (int i = 0; hWeapons[i]; i++)
    {
        if (IDX_BAD(HandleToIDX(hWeapons[i])))
            continue;
        // Get the weapon
        CachedEntity *weapon = ENTITY(HandleToIDX(hWeapons[i]));
        // if weapon is what we are looking for, return true
        if (CE_VALID(weapon) && re::C_TFWeaponBase::GetWeaponID(RAW_ENT(weapon)) == weaponid)
            return weapon->m_IDX;
    }
    // Nothing found
    return -1;
}

// A function to tell if a player is using a specific weapon
bool HasWeapon(CachedEntity *ent, int wantedId)
{
    if (CE_BAD(ent) || ent->m_Type() != ENTITY_PLAYER)
        return false;
    // Grab the handle and store it into the var
    int *hWeapons = &CE_INT(ent, netvar.hMyWeapons);
    if (!hWeapons)
        return false;
    // Go through the handle array and search for the item
    for (int i = 0; hWeapons[i]; i++)
    {
        if (IDX_BAD(HandleToIDX(hWeapons[i])))
            continue;
        // Get the weapon
        CachedEntity *weapon = ENTITY(HandleToIDX(hWeapons[i]));
        // if weapon is what we are looking for, return true
        if (CE_VALID(weapon) && CE_INT(weapon, netvar.iItemDefinitionIndex) == wantedId)
            return true;
    }
    // We didnt find the weapon we needed, return false
    return false;
}

CachedEntity *getClosestEntity(Vector vec)
{
    float distance         = FLT_MAX;
    CachedEntity *best_ent = nullptr;
    for (int i = 1; i <= g_IEngine->GetMaxClients(); i++)
    {
        CachedEntity *ent = ENTITY(i);
        if (CE_VALID(ent) && ent->m_vecDormantOrigin() && ent->m_bAlivePlayer() && ent->m_bEnemy() && vec.DistTo(ent->m_vecOrigin()) < distance)
        {
            distance = vec.DistTo(*ent->m_vecDormantOrigin());
            best_ent = ent;
        }
    }
    return best_ent;
}

CachedEntity *getClosestNonlocalEntity(Vector vec)
{
    float distance         = FLT_MAX;
    CachedEntity *best_ent = nullptr;
    for (int i = 1; i <= g_IEngine->GetMaxClients(); i++)
    {
        CachedEntity *ent = ENTITY(i);
        if (CE_VALID(ent) && ent->m_IDX != g_pLocalPlayer->entity_idx && ent->m_vecDormantOrigin() && ent->m_bAlivePlayer() && ent->m_bEnemy() && vec.DistTo(ent->m_vecOrigin()) < distance)
        {
            distance = vec.DistTo(*ent->m_vecDormantOrigin());
            best_ent = ent;
        }
    }
    return best_ent;
}

void VectorTransform(const float *in1, const matrix3x4_t &in2, float *out)
{
    out[0] = (in1[0] * in2[0][0] + in1[1] * in2[0][1] + in1[2] * in2[0][2]) + in2[0][3];
    out[1] = (in1[0] * in2[1][0] + in1[1] * in2[1][1] + in1[2] * in2[1][2]) + in2[1][3];
    out[2] = (in1[0] * in2[2][0] + in1[1] * in2[2][1] + in1[2] * in2[2][2]) + in2[2][3];
}

bool GetHitbox(CachedEntity *entity, int hb, Vector &out)
{
    hitbox_cache::CachedHitbox *box;

    if (CE_BAD(entity))
        return false;
    box = entity->hitboxes.GetHitbox(hb);
    if (!box)
        out = entity->m_vecOrigin();
    else
        out = box->center;
    return true;
}

void MatrixGetColumn(const matrix3x4_t &in, int column, Vector &out)
{
    out.x = in[0][column];
    out.y = in[1][column];
    out.z = in[2][column];
}

void MatrixAngles(const matrix3x4_t &matrix, float *angles)
{
    float forward[3];
    float left[3];
    float up[3];

    //
    // Extract the basis vectors from the matrix. Since we only need the Z
    // component of the up vector, we don't get X and Y.
    //
    forward[0] = matrix[0][0];
    forward[1] = matrix[1][0];
    forward[2] = matrix[2][0];
    left[0]    = matrix[0][1];
    left[1]    = matrix[1][1];
    left[2]    = matrix[2][1];
    up[2]      = matrix[2][2];

    float xyDist = std::sqrt(forward[0] * forward[0] + forward[1] * forward[1]);

    // enough here to get angles?
    if (xyDist > 0.001f)
    {
        // (yaw)    y = ATAN( forward.y, forward.x );       -- in our space, forward is the X axis
        angles[1] = RAD2DEG(atan2f(forward[1], forward[0]));

        // (pitch)  x = ATAN( -forward.z, sqrt(forward.x*forward.x+forward.y*forward.y) );
        angles[0] = RAD2DEG(atan2f(-forward[2], xyDist));

        // (roll)   z = ATAN( left.z, up.z );
        angles[2] = RAD2DEG(atan2f(left[2], up[2]));
    }
    else
    {
        // (yaw)    y = ATAN( -left.x, left.y );            -- forward is mostly z, so use right for yaw
        angles[1] = RAD2DEG(atan2f(-left[0], left[1]));

        // (pitch)  x = ATAN( -forward.z, sqrt(forward.x*forward.x+forward.y*forward.y) );
        angles[0] = RAD2DEG(atan2f(-forward[2], xyDist));

        // Assume no roll in this case as one degree of freedom has been lost (i.e. yaw == roll)
        angles[2] = 0;
    }
}

void VectorAngles(Vector &forward, Vector &angles)
{
    float tmp, yaw, pitch;

    if (forward[1] == 0 && forward[0] == 0)
    {
        yaw = 0;
        if (forward[2] > 0)
            pitch = 270;
        else
            pitch = 90;
    }
    else
    {
        yaw = (atan2(forward[1], forward[0]) * 180 / PI);
        if (yaw < 0)
            yaw += 360;

        tmp   = sqrt((forward[0] * forward[0] + forward[1] * forward[1]));
        pitch = (atan2(-forward[2], tmp) * 180 / PI);
        if (pitch < 0)
            pitch += 360;
    }

    angles[0] = pitch;
    angles[1] = yaw;
    angles[2] = 0;
}

// Forward, right, and up
void AngleVectors3(const QAngle &angles, Vector *forward, Vector *right, Vector *up)
{
    float sr, sp, sy, cr, cp, cy;
    SinCos(DEG2RAD(angles[YAW]), &sy, &cy);
    SinCos(DEG2RAD(angles[PITCH]), &sp, &cp);
    SinCos(DEG2RAD(angles[ROLL]), &sr, &cr);

    if (forward)
    {
        forward->x = cp * cy;
        forward->y = cp * sy;
        forward->z = -sp;
    }

    if (right)
    {
        right->x = (-1 * sr * sp * cy + -1 * cr * -sy);
        right->y = (-1 * sr * sp * sy + -1 * cr * cy);
        right->z = -1 * sr * cp;
    }

    if (up)
    {
        up->x = (cr * sp * cy + -sr * -sy);
        up->y = (cr * sp * sy + -sr * cy);
        up->z = cr * cp;
    }
}

bool isRapidFire(IClientEntity *wep)
{
    weapon_info info(wep);
    bool ret = GetWeaponData(wep)->m_bUseRapidFireCrits;
    // Minigun changes mode once revved, so fix that
    return ret || wep->GetClientClass()->m_ClassID == CL_CLASS(CTFMinigun);
}

// Get forward vector
void AngleVectors2(const QAngle &angles, Vector *forward)
{
    float sp, sy, cp, cy;

    SinCos(DEG2RAD(angles[YAW]), &sy, &cy);
    SinCos(DEG2RAD(angles[PITCH]), &sp, &cp);

    forward->x = cp * cy;
    forward->y = cp * sy;
    forward->z = -sp;
}

char GetUpperChar(ButtonCode_t button)
{
    switch (button)
    {
    case KEY_0:
        return ')';
    case KEY_1:
        return '!';
    case KEY_2:
        return '@';
    case KEY_3:
        return '#';
    case KEY_4:
        return '$';
    case KEY_5:
        return '%';
    case KEY_6:
        return '^';
    case KEY_7:
        return '&';
    case KEY_8:
        return '*';
    case KEY_9:
        return '(';
    case KEY_LBRACKET:
        return '{';
    case KEY_RBRACKET:
        return '}';
    case KEY_SEMICOLON:
        return ':';
    case KEY_BACKQUOTE:
        return '~';
    case KEY_APOSTROPHE:
        return '"';
    case KEY_COMMA:
        return '<';
    case KEY_PERIOD:
        return '>';
    case KEY_SLASH:
        return '?';
    case KEY_BACKSLASH:
        return '|';
    case KEY_MINUS:
        return '_';
    case KEY_EQUAL:
        return '+';
    default:
        if (strlen(g_IInputSystem->ButtonCodeToString(button)) != 1)
            return 0;
        return toupper(*g_IInputSystem->ButtonCodeToString(button));
    }
}

char GetChar(ButtonCode_t button)
{
    switch (button)
    {
    case KEY_PAD_DIVIDE:
        return '/';
    case KEY_PAD_MULTIPLY:
        return '*';
    case KEY_PAD_MINUS:
        return '-';
    case KEY_PAD_PLUS:
        return '+';
    case KEY_SEMICOLON:
        return ';';
    default:
        if (button >= KEY_PAD_0 && button <= KEY_PAD_9)
        {
            return button - KEY_PAD_0 + '0';
        }
        if (strlen(g_IInputSystem->ButtonCodeToString(button)) != 1)
            return 0;
        return *g_IInputSystem->ButtonCodeToString(button);
    }
}

void FixMovement(CUserCmd &cmd, Vector &viewangles)
{
    Vector movement, ang;
    float speed, yaw;
    movement.x = cmd.forwardmove;
    movement.y = cmd.sidemove;
    movement.z = cmd.upmove;
    speed      = sqrt(movement.x * movement.x + movement.y * movement.y);
    VectorAngles(movement, ang);
    yaw             = DEG2RAD(ang.y - viewangles.y + cmd.viewangles.y);
    cmd.forwardmove = cos(yaw) * speed;
    cmd.sidemove    = sin(yaw) * speed;
}

bool AmbassadorCanHeadshot()
{
    if (IsAmbassador(g_pLocalPlayer->weapon()))
    {
        if ((g_GlobalVars->curtime - CE_FLOAT(g_pLocalPlayer->weapon(), netvar.flLastFireTime)) <= 1.0)
        {
            return false;
        }
    }
    return true;
}

static std::random_device random_device;
static std::mt19937 engine{ random_device() };

float RandFloatRange(float min, float max)
{
    std::uniform_real_distribution<float> random(min, max);
    return random(engine);
}

int UniformRandomInt(int min, int max)
{
    std::uniform_int_distribution<int> random(min, max);
    return random(engine);
}

bool IsEntityVisible(CachedEntity *entity, int hb)
{
    if (g_Settings.bInvalid)
        return false;
    if (entity == g_pLocalPlayer->entity)
        return true;
    if (hb == -1)
        return IsEntityVectorVisible(entity, entity->m_vecOrigin());
    else
        return entity->hitboxes.VisibilityCheck(hb);
}

std::mutex trace_lock;
bool IsEntityVectorVisible(CachedEntity *entity, Vector endpos, bool use_weapon_offset, unsigned int mask, trace_t *trace)
{
    trace_t trace_object;
    if (!trace)
        trace = &trace_object;
    Ray_t ray;

    if (g_Settings.bInvalid)
        return false;
    if (entity == g_pLocalPlayer->entity)
        return true;
    if (CE_BAD(g_pLocalPlayer->entity))
        return false;
    if (CE_BAD(entity))
        return false;
    trace::filter_default.SetSelf(RAW_ENT(g_pLocalPlayer->entity));
    Vector eye = g_pLocalPlayer->v_Eye;
    // Adjust for weapon offsets if needed
    if (use_weapon_offset)
        eye = getShootPos(GetAimAtAngles(eye, endpos, LOCAL_E));
    ray.Init(eye, endpos);
    {
        PROF_SECTION(IEVV_TraceRay);
        std::lock_guard<std::mutex> lock(trace_lock);
        if (!tcm || g_Settings.is_create_move)
            g_ITrace->TraceRay(ray, mask, &trace::filter_default, trace);
    }
    return (((IClientEntity *) trace->m_pEnt) == RAW_ENT(entity) || !trace->DidHit());
}

// Get all the corners of a box. Taken from sauce engine.
void GenerateBoxVertices(const Vector &vOrigin, const QAngle &angles, const Vector &vMins, const Vector &vMaxs, Vector pVerts[8])
{
    // Build a rotation matrix from orientation
    matrix3x4_t fRotateMatrix;
    AngleMatrix(angles, fRotateMatrix);

    Vector vecPos;
    for (int i = 0; i < 8; ++i)
    {
        vecPos[0] = (i & 0x1) ? vMaxs[0] : vMins[0];
        vecPos[1] = (i & 0x2) ? vMaxs[1] : vMins[1];
        vecPos[2] = (i & 0x4) ? vMaxs[2] : vMins[2];

        VectorRotate(vecPos, fRotateMatrix, pVerts[i]);
        pVerts[i] += vOrigin;
    }
}

// For when you need to vis check something that isnt the local player
bool VisCheckEntFromEnt(CachedEntity *startEnt, CachedEntity *endEnt)
{
    // We setSelf as the starting ent as we dont want to hit it, we want the
    // other ent
    trace_t trace;
    trace::filter_default.SetSelf(RAW_ENT(startEnt));

    // Setup the trace starting with the origin of the starting ent attemting to
    // hit the origin of the end ent
    Ray_t ray;
    ray.Init(startEnt->m_vecOrigin(), endEnt->m_vecOrigin());
    {
        PROF_SECTION(IEVV_TraceRay);
        g_ITrace->TraceRay(ray, MASK_SHOT_HULL, &trace::filter_default, &trace);
    }
    // Is the entity that we hit our target ent? if so, the vis check passes
    if (trace.m_pEnt)
    {
        if ((((IClientEntity *) trace.m_pEnt)) == RAW_ENT(endEnt))
            return true;
    }
    // Since we didnt hit our target ent, the vis check failed so return false
    return false;
}

// Use when you need to vis check something but its not the ent origin that you
// use, so we check from the vector to the ent, ignoring the first just in case
bool VisCheckEntFromEntVector(Vector startVector, CachedEntity *startEnt, CachedEntity *endEnt)
{
    // We setSelf as the starting ent as we dont want to hit it, we want the
    // other ent
    trace_t trace;
    trace::filter_default.SetSelf(RAW_ENT(startEnt));

    // Setup the trace starting with the origin of the starting ent attemting to
    // hit the origin of the end ent
    Ray_t ray;
    ray.Init(startVector, endEnt->m_vecOrigin());
    {
        PROF_SECTION(IEVV_TraceRay);
        g_ITrace->TraceRay(ray, MASK_SHOT_HULL, &trace::filter_default, &trace);
    }
    // Is the entity that we hit our target ent? if so, the vis check passes
    if (trace.m_pEnt)
    {
        if ((((IClientEntity *) trace.m_pEnt)) == RAW_ENT(endEnt))
            return true;
    }
    // Since we didnt hit our target ent, the vis check failed so return false
    return false;
}

Vector GetBuildingPosition(CachedEntity *ent)
{
    // Get a centered hitbox
    auto hitbox = ent->hitboxes.GetHitbox(std::max(0, ent->hitboxes.GetNumHitboxes() / 2 - 1));
    // Dormant/Invalid, return origin
    if (!hitbox)
        return ent->m_vecOrigin();
    return hitbox->center;
}

bool IsBuildingVisible(CachedEntity *ent)
{
    return IsEntityVectorVisible(ent, GetBuildingPosition(ent));
}

int HandleToIDX(int handle)
{
    return handle & 0xFFF;
}

void fClampAngle(Vector &qaAng)
{
    while (qaAng[0] > 89)
        qaAng[0] -= 180;

    while (qaAng[0] < -89)
        qaAng[0] += 180;

    while (qaAng[1] > 180)
        qaAng[1] -= 360;

    while (qaAng[1] < -180)
        qaAng[1] += 360;

    qaAng.z = 0;
}

float DistToSqr(CachedEntity *entity)
{
    if (CE_BAD(entity))
        return 0.0f;
    return g_pLocalPlayer->v_Origin.DistToSqr(entity->m_vecOrigin());
}

bool IsProjectileCrit(CachedEntity *ent)
{
    if (ent->m_bGrenadeProjectile())
        return CE_BYTE(ent, netvar.Grenade_bCritical);
    return CE_BYTE(ent, netvar.Rocket_bCritical);
}

CachedEntity *weapon_get(CachedEntity *entity)
{
    int handle, eid;

    if (CE_BAD(entity))
        return 0;
    handle = CE_INT(entity, netvar.hActiveWeapon);
    eid    = handle & 0xFFF;
    if (IDX_BAD(eid))
        return 0;
    return ENTITY(eid);
}

weaponmode GetWeaponMode(CachedEntity *ent)
{
    int weapon_handle, slot;
    CachedEntity *weapon;

    if (CE_BAD(ent) || CE_BAD(weapon_get(ent)))
        return weapon_invalid;
    weapon_handle = CE_INT(ent, netvar.hActiveWeapon);
    if (IDX_BAD((weapon_handle & 0xFFF)))
    {
        // logging::Info("IDX_BAD: %i", weapon_handle & 0xFFF);
        return weaponmode::weapon_invalid;
    }
    weapon = (ENTITY(weapon_handle & 0xFFF));
    if (CE_BAD(weapon))
        return weaponmode::weapon_invalid;
    int classid = weapon->m_iClassID();
    slot        = re::C_BaseCombatWeapon::GetSlot(RAW_ENT(weapon));
    if (slot == 2)
        return weaponmode::weapon_melee;
    if (slot > 2)
        return weaponmode::weapon_pda;
    switch (classid)
    {
    case CL_CLASS(CTFLunchBox):
    case CL_CLASS(CTFLunchBox_Drink):
    case CL_CLASS(CTFBuffItem):
        return weaponmode::weapon_consumable;
    case CL_CLASS(CTFRocketLauncher_DirectHit):
    case CL_CLASS(CTFRocketLauncher):
    case CL_CLASS(CTFGrenadeLauncher):
    case CL_CLASS(CTFPipebombLauncher):
    case CL_CLASS(CTFCompoundBow):
    case CL_CLASS(CTFBat_Wood):
    case CL_CLASS(CTFBat_Giftwrap):
    case CL_CLASS(CTFFlareGun):
    case CL_CLASS(CTFFlareGun_Revenge):
    case CL_CLASS(CTFSyringeGun):
    case CL_CLASS(CTFCrossbow):
    case CL_CLASS(CTFShotgunBuildingRescue):
    case CL_CLASS(CTFDRGPomson):
    case CL_CLASS(CTFWeaponFlameBall):
    case CL_CLASS(CTFRaygun):
    case CL_CLASS(CTFGrapplingHook):
    case CL_CLASS(CTFParticleCannon): // Cow Mangler 5000
    case CL_CLASS(CTFRocketLauncher_AirStrike):
    case CL_CLASS(CTFCannon):
    case CL_CLASS(CTFMechanicalArm):
        return weaponmode::weapon_projectile;
    case CL_CLASS(CTFJar):
    case CL_CLASS(CTFJarMilk):
    case CL_CLASS(CTFJarGas):
    case CL_CLASS(CTFCleaver):
        return weaponmode::weapon_throwable;
    case CL_CLASS(CWeaponMedigun):
        return weaponmode::weapon_medigun;
    case CL_CLASS(CTFWeaponSapper):
    case CL_CLASS(CTFWeaponPDA):
    case CL_CLASS(CTFWeaponPDA_Spy):
    case CL_CLASS(CTFWeaponPDA_Engineer_Build):
    case CL_CLASS(CTFWeaponPDA_Engineer_Destroy):
    case CL_CLASS(CTFWeaponBuilder):
    case CL_CLASS(CTFLaserPointer):
        return weaponmode::weapon_pda;
    default:
        return weaponmode::weapon_hitscan;
    }
}

weaponmode GetWeaponMode()
{
    return g_pLocalPlayer->weapon_mode;
}

bool LineIntersectsBox(Vector &bmin, Vector &bmax, Vector &lmin, Vector &lmax)
{
    if (lmax.x < bmin.x && lmin.x < bmin.x)
        return false;
    if (lmax.y < bmin.y && lmin.y < bmin.y)
        return false;
    if (lmax.z < bmin.z && lmin.z < bmin.z)
        return false;
    if (lmax.x > bmax.x && lmin.x > bmax.x)
        return false;
    if (lmax.y > bmax.y && lmin.y > bmax.y)
        return false;
    if (lmax.z > bmax.z && lmin.z > bmax.z)
        return false;
    return true;
}

bool GetProjectileData(CachedEntity *weapon, float &speed, float &gravity, float &start_velocity)
{
    float rspeed, rgrav, rinitial_vel;

    IF_GAME(!IsTF()) return false;

    if (CE_BAD(weapon))
        return false;
    rspeed       = 0.0f;
    rgrav        = 0.0f;
    rinitial_vel = 0.0f;

    switch (weapon->m_iClassID())
    {
    case CL_CLASS(CTFRocketLauncher_DirectHit):
    {
        rspeed = 1980.0f;
        break;
    }
    case CL_CLASS(CTFParticleCannon): // Cow Mangler 5000
    case CL_CLASS(CTFRocketLauncher_AirStrike):
    case CL_CLASS(CTFRocketLauncher):
    {
        rspeed = 1100.0f;
        // Libery Launcher
        if (CE_INT(weapon, netvar.iItemDefinitionIndex) == 414)
            rspeed = 1540.0f;
        break;
    }
    case CL_CLASS(CTFCannon):
    {
        rspeed = 1453.9f;
        break;
    }
    case CL_CLASS(CTFGrenadeLauncher):
    {
        rspeed       = 1216.6f;
        rgrav        = 1.0f;
        rinitial_vel = 200.0f;
        IF_GAME(IsTF2())
        {
            // Loch'n Load
            if (CE_INT(weapon, netvar.iItemDefinitionIndex) == 308)
                rspeed = 1513.3f;
        }
        break;
    }
    case CL_CLASS(CTFPipebombLauncher):
    {
        float chargetime = g_GlobalVars->curtime - CE_FLOAT(weapon, netvar.flChargeBeginTime);
        if (!CE_FLOAT(weapon, netvar.flChargeBeginTime))
            chargetime = 0.0f;
        rspeed       = RemapValClamped(chargetime, 0.0f, 4.0f, 925.38, 2409.2);
        rgrav        = 1.0f;
        rinitial_vel = 200.0f;
        if (CE_INT(weapon, netvar.iItemDefinitionIndex) == 1150) // Quickiebomb Launcher
            rspeed = RemapValClamped(chargetime, 0.0f, 4.0f, 930.88, 2409.2);
        break;
    }
    case CL_CLASS(CTFCompoundBow):
    {
        float chargetime = g_GlobalVars->curtime - CE_FLOAT(weapon, netvar.flChargeBeginTime);
        rspeed           = RemapValClamped(chargetime, 0.0f, 1.f, 1800, 2600);
        rgrav            = RemapValClamped(chargetime, 0.0f, 1.f, 0.5, 0.1) - 0.05;
        break;
    }
    case CL_CLASS(CTFBat_Giftwrap):
    case CL_CLASS(CTFBat_Wood):
    {
        rspeed = 3000.0f;
        rgrav  = 0.5f;
        break;
    }
    case CL_CLASS(CTFFlareGun):
    case CL_CLASS(CTFFlareGun_Revenge): // Detonator
    {
        rspeed = 2000.0f;
        rgrav  = 0.25f;
        break;
    }
    case CL_CLASS(CTFSyringeGun):
    {
        rgrav  = 0.2f;
        rspeed = 1000.0f;
        break;
    }
    case CL_CLASS(CTFCrossbow):
    case CL_CLASS(CTFShotgunBuildingRescue):
    {
        rgrav  = 0.2f;
        rspeed = 2400.0f;
        break;
    }
    case CL_CLASS(CTFDRGPomson):
    case CL_CLASS(CTFRaygun): // Righteous Bison
    {
        rspeed = 1200.0f;
        break;
    }
    case CL_CLASS(CTFWeaponFlameBall):
    case CL_CLASS(CTFCleaver):
    {
        rspeed = 3000.0f;
        break;
    }
    case CL_CLASS(CTFGrapplingHook):
    {
        rspeed = 1500.0f;
        break;
    }
    case CL_CLASS(CTFJarMilk):
    {
        rspeed = 1019.9f;
        break;
    }
    case CL_CLASS(CTFJar):
    {
        rspeed = 1017.9f;
        break;
    }
    case CL_CLASS(CTFJarGas):
    {
        rspeed = 2009.2f;
        break;
    }
    }

    speed          = rspeed;
    gravity        = rgrav;
    start_velocity = rinitial_vel;
    return (rspeed || rgrav || rinitial_vel);
}

/*const char* MakeInfoString(IClientEntity* player) {
    char* buf = new char[256]();
    player_info_t info;
    if (!engineClient->GetPlayerInfo(player->entindex(), &info)) return (const
char*)0; logging::Info("a"); int hWeapon = NET_INT(player,
netvar.hActiveWeapon); if (NET_BYTE(player, netvar.iLifeState)) { sprintf(buf,
"%s is dead %s", info.name, tfclasses[NET_INT(player, netvar.iClass)]); return
buf;
    }
    if (hWeapon) {
        IClientEntity* weapon = ENTITY(hWeapon & 0xFFF);
        sprintf(buf, "%s is %s with %i health using %s", info.name,
tfclasses[NET_INT(player, netvar.iClass)], NET_INT(player, netvar.iHealth),
weapon->GetClientClass()->GetName()); } else { sprintf(buf, "%s is %s with %i
health", info.name, tfclasses[NET_INT(player, netvar.iClass)], NET_INT(player,
netvar.iHealth));
    }
    logging::Info("Result: %s", buf);
    return buf;
}*/

bool IsVectorVisible(Vector origin, Vector target, bool enviroment_only, CachedEntity *self, unsigned int mask)
{
    if (!enviroment_only)
    {
        trace_t trace_visible;
        Ray_t ray;

        trace::filter_no_player.SetSelf(RAW_ENT(self));
        ray.Init(origin, target);
        PROF_SECTION(IEVV_TraceRay);
        g_ITrace->TraceRay(ray, mask, &trace::filter_no_player, &trace_visible);
        return (trace_visible.fraction == 1.0f);
    }
    else
    {
        trace_t trace_visible;
        Ray_t ray;

        trace::filter_no_entity.SetSelf(RAW_ENT(self));
        ray.Init(origin, target);
        PROF_SECTION(IEVV_TraceRay);
        g_ITrace->TraceRay(ray, mask, &trace::filter_no_entity, &trace_visible);
        return (trace_visible.fraction == 1.0f);
    }
}

bool IsVectorVisibleNavigation(Vector origin, Vector target, unsigned int mask)
{
    trace_t trace_visible;
    Ray_t ray;

    ray.Init(origin, target);
    PROF_SECTION(IEVV_TraceRay);
    g_ITrace->TraceRay(ray, mask, &trace::filter_navigation, &trace_visible);
    return (trace_visible.fraction == 1.0f);
}

void WhatIAmLookingAt(int *result_eindex, Vector *result_pos)
{
    Ray_t ray;
    Vector forward;
    float sp, sy, cp, cy;
    QAngle angle;
    trace_t trace;

    trace::filter_default.SetSelf(RAW_ENT(g_pLocalPlayer->entity));
    g_IEngine->GetViewAngles(angle);
    sy        = sinf(DEG2RAD(angle[1]));
    cy        = cosf(DEG2RAD(angle[1]));
    sp        = sinf(DEG2RAD(angle[0]));
    cp        = cosf(DEG2RAD(angle[0]));
    forward.x = cp * cy;
    forward.y = cp * sy;
    forward.z = -sp;
    forward   = forward * 8192.0f + g_pLocalPlayer->v_Eye;
    ray.Init(g_pLocalPlayer->v_Eye, forward);
    {
        PROF_SECTION(IEVV_TraceRay);
        g_ITrace->TraceRay(ray, 0x4200400B, &trace::filter_default, &trace);
    }
    if (result_pos)
        *result_pos = trace.endpos;
    if (result_eindex)
        *result_eindex = -1;
    if (trace.m_pEnt && result_eindex)
        *result_eindex = ((IClientEntity *) (trace.m_pEnt))->entindex();
}

Vector GetForwardVector(Vector origin, Vector viewangles, float distance, CachedEntity *punch_entity)
{
    Vector forward;
    float sp, sy, cp, cy;
    QAngle angle = VectorToQAngle(viewangles);
    // Compensate for punch angle
    if (punch_entity && should_correct_punch)
        angle += VectorToQAngle(CE_VECTOR(punch_entity, netvar.vecPunchAngle));

    sy        = sinf(DEG2RAD(angle[1]));
    cy        = cosf(DEG2RAD(angle[1]));
    sp        = sinf(DEG2RAD(angle[0]));
    cp        = cosf(DEG2RAD(angle[0]));
    forward.x = cp * cy;
    forward.y = cp * sy;
    forward.z = -sp;
    forward   = forward * distance + origin;
    return forward;
}

Vector GetForwardVector(float distance, CachedEntity *punch_entity)
{
    return GetForwardVector(g_pLocalPlayer->v_Eye, g_pLocalPlayer->v_OrigViewangles, distance, punch_entity);
}

bool IsSentryBuster(CachedEntity *entity)
{
    return (entity->m_Type() == EntityType::ENTITY_PLAYER && CE_INT(entity, netvar.iClass) == tf_class::tf_demoman && g_pPlayerResource->GetMaxHealth(entity) == 2500);
}

bool IsAmbassador(CachedEntity *entity)
{
    IF_GAME(!IsTF2()) return false;
    if (entity->m_iClassID() != CL_CLASS(CTFRevolver))
        return false;
    const int &defidx = CE_INT(entity, netvar.iItemDefinitionIndex);
    return (defidx == 61 || defidx == 1006);
}

bool IsPlayerInvulnerable(CachedEntity *player)
{
    return HasConditionMask<KInvulnerabilityMask.cond_0, KInvulnerabilityMask.cond_1, KInvulnerabilityMask.cond_2, KInvulnerabilityMask.cond_3>(player);
}

bool IsPlayerCritBoosted(CachedEntity *player)
{
    return HasConditionMask<KCritBoostMask.cond_0, KCritBoostMask.cond_1, KCritBoostMask.cond_2, KCritBoostMask.cond_3>(player);
}

bool IsPlayerInvisible(CachedEntity *player)
{
    return HasConditionMask<KInvisibilityMask.cond_0, KInvisibilityMask.cond_1, KInvisibilityMask.cond_2, KInvisibilityMask.cond_3>(player);
}

bool IsPlayerDisguised(CachedEntity *player)
{
    return HasConditionMask<KDisguisedMask.cond_0, KDisguisedMask.cond_1, KDisguisedMask.cond_2, KDisguisedMask.cond_3>(player);
}

bool IsPlayerResistantToCurrentWeapon(CachedEntity *player)
{
    switch (LOCAL_W->m_iClassID())
    {
    case CL_CLASS(CTFRocketLauncher_DirectHit):
    case CL_CLASS(CTFRocketLauncher_AirStrike):
    case CL_CLASS(CTFRocketLauncher_Mortar): // doesn't exist yet
    case CL_CLASS(CTFRocketLauncher):
    case CL_CLASS(CTFParticleCannon):
    case CL_CLASS(CTFGrenadeLauncher):
    case CL_CLASS(CTFPipebombLauncher):
        if (HasCondition<TFCond_UberBlastResist>(player))
            return true;
        break;
    case CL_CLASS(CTFCompoundBow):
    case CL_CLASS(CTFSyringeGun):
    case CL_CLASS(CTFCrossbow):
    case CL_CLASS(CTFShotgunBuildingRescue):
    case CL_CLASS(CTFDRGPomson):
    case CL_CLASS(CTFRaygun):
        if (HasCondition<TFCond_UberBulletResist>(player))
            return true;
        break;
    case CL_CLASS(CTFWeaponFlameBall):
    case CL_CLASS(CTFFlareGun):
    case CL_CLASS(CTFFlareGun_Revenge):
    case CL_CLASS(CTFFlameRocket):
    case CL_CLASS(CTFFlameThrower):
        if (HasCondition<TFCond_UberFireResist>(player))
            return true;
        break;
    default:
        if (g_pLocalPlayer->weapon_mode == weaponmode::weapon_hitscan && HasCondition<TFCond_UberBulletResist>(player))
            return true;
    }
    return false;
}

// F1 c&p
Vector CalcAngle(Vector src, Vector dst)
{
    Vector AimAngles, delta;
    float hyp;
    delta       = src - dst;
    hyp         = sqrtf((delta.x * delta.x) + (delta.y * delta.y)); // SUPER SECRET IMPROVEMENT CODE NAME DONUT STEEL
    AimAngles.x = atanf(delta.z / hyp) * RADPI;
    AimAngles.y = atanf(delta.y / delta.x) * RADPI;
    AimAngles.z = 0.0f;
    if (delta.x >= 0.0)
        AimAngles.y += 180.0f;
    return AimAngles;
}

void MakeVector(Vector angle, Vector &vector)
{
    float pitch, yaw, tmp;
    pitch     = float(angle[0] * PI / 180);
    yaw       = float(angle[1] * PI / 180);
    tmp       = float(cos(pitch));
    vector[0] = float(-tmp * -cos(yaw));
    vector[1] = float(sin(yaw) * tmp);
    vector[2] = float(-sin(pitch));
}

float GetFov(Vector angle, Vector src, Vector dst)
{
    Vector ang, aim;
    float mag, u_dot_v;
    ang = CalcAngle(src, dst);

    MakeVector(angle, aim);
    MakeVector(ang, ang);

    mag     = sqrtf(pow(aim.x, 2) + pow(aim.y, 2) + pow(aim.z, 2));
    u_dot_v = aim.Dot(ang);

    // Congratulations! you managed to go out of domain. That means you are directly on the target
    // And floating point inprecision breaks this function making it return NAN, so we "fix" it via this.
    if (u_dot_v / (pow(mag, 2)) > 1.0f)
        return 0;

    return RAD2DEG(acos(u_dot_v / (pow(mag, 2))));
}

bool CanHeadshot()
{
    return (g_pLocalPlayer->flZoomBegin > 0.0f && (g_GlobalVars->curtime - g_pLocalPlayer->flZoomBegin > 0.2f));
}

bool CanShoot()
{
    // PrecalculateCanShoot() CreateMove.cpp
    return calculated_can_shoot;
}

float ATTRIB_HOOK_FLOAT(float base_value, const char *search_string, IClientEntity *ent, void *buffer, bool is_global_const_string)
{
    typedef float (*AttribHookFloat_t)(float, const char *, IClientEntity *, void *, bool);

    static uintptr_t AttribHookFloat = e8call_direct(gSignatures.GetClientSignature("E8 ? ? ? ? 8B 96 ? ? ? ? D9 5D"));
    static auto AttribHookFloat_fn   = AttribHookFloat_t(AttribHookFloat);

    return AttribHookFloat_fn(base_value, search_string, ent, buffer, is_global_const_string);
}

QAngle VectorToQAngle(Vector in)
{
    return *(QAngle *) &in;
}

Vector QAngleToVector(QAngle in)
{
    return *(Vector *) &in;
}

void AimAt(Vector origin, Vector target, CUserCmd *cmd, bool compensate_punch)
{
    cmd->viewangles = GetAimAtAngles(origin, target, compensate_punch ? LOCAL_E : nullptr);
}

void AimAtHitbox(CachedEntity *ent, int hitbox, CUserCmd *cmd, bool compensate_punch)
{
    Vector r;
    r = ent->m_vecOrigin();
    GetHitbox(ent, hitbox, r);
    AimAt(g_pLocalPlayer->v_Eye, r, cmd, compensate_punch);
}

void FastStop()
{
    // Get velocity
    Vector vel;
    velocity::EstimateAbsVelocity(RAW_ENT(LOCAL_E), vel);

    static auto sv_friction  = g_ICvar->FindVar("sv_friction");
    static auto sv_stopspeed = g_ICvar->FindVar("sv_stopspeed");

    auto speed    = vel.Length2D();
    auto friction = sv_friction->GetFloat() * CE_FLOAT(LOCAL_E, 0x12b8);
    auto control  = (speed < sv_stopspeed->GetFloat()) ? sv_stopspeed->GetFloat() : speed;
    auto drop     = control * friction * g_GlobalVars->interval_per_tick;

    if (speed > drop - 1.0f)
    {
        Vector velocity = vel;
        Vector direction;
        VectorAngles(vel, direction);
        float speed = velocity.Length();

        direction.y = current_user_cmd->viewangles.y - direction.y;

        Vector forward;
        AngleVectors2(VectorToQAngle(direction), &forward);
        Vector negated_direction = forward * -speed;

        current_user_cmd->forwardmove = negated_direction.x;
        current_user_cmd->sidemove    = negated_direction.y;
    }
    else
    {
        current_user_cmd->forwardmove = current_user_cmd->sidemove = 0.0f;
    }
}

bool IsEntityVisiblePenetration(CachedEntity *entity, int hb)
{
    trace_t trace_visible;
    Ray_t ray;
    Vector hit;
    int ret;
    bool correct_entity;
    IClientEntity *ent;
    trace::filter_penetration.SetSelf(RAW_ENT(g_pLocalPlayer->entity));
    trace::filter_penetration.Reset();
    ret = GetHitbox(entity, hb, hit);
    if (ret)
    {
        return false;
    }
    ray.Init(g_pLocalPlayer->v_Origin + g_pLocalPlayer->v_ViewOffset, hit);
    {
        PROF_SECTION(IEVV_TraceRay);
        g_ITrace->TraceRay(ray, MASK_SHOT_HULL, &trace::filter_penetration, &trace_visible);
    }
    correct_entity = false;
    if (trace_visible.m_pEnt)
    {
        correct_entity = ((IClientEntity *) trace_visible.m_pEnt) == RAW_ENT(entity);
    }
    if (!correct_entity)
        return false;
    {
        PROF_SECTION(IEVV_TraceRay);
        g_ITrace->TraceRay(ray, 0x4200400B, &trace::filter_default, &trace_visible);
    }
    if (trace_visible.m_pEnt)
    {
        ent = (IClientEntity *) trace_visible.m_pEnt;
        if (ent)
        {
            if (ent->GetClientClass()->m_ClassID == RCC_PLAYER)
            {
                if (ent == RAW_ENT(entity))
                    return false;
                if (trace_visible.hitbox >= 0)
                {
                    return true;
                }
            }
        }
    }
    return false;
}

void ValidateUserCmd(CUserCmd *cmd, int sequence_nr)
{
    CRC32_t crc = GetChecksum(cmd);
    if (crc != GetVerifiedCmds(g_IInput)[sequence_nr % VERIFIED_CMD_SIZE].m_crc)
    {
        *cmd = GetVerifiedCmds(g_IInput)[sequence_nr % VERIFIED_CMD_SIZE].m_cmd;
    }
}

// Used for getting class names
CatCommand print_classnames("debug_print_classnames", "Lists classnames currently available in console", []() {
    // Create a tmp ent for the loop
    CachedEntity *ent;

    // Go through all the entities
    for (int i = 0; i <= HIGHEST_ENTITY; i++)
    {

        // Get an entity
        ent = ENTITY(i);
        // Check for null/dormant
        if (CE_BAD(ent))
            continue;

        // Print in console, the class name of the ent
        logging::Info(format(RAW_ENT(ent)->GetClientClass()->m_pNetworkName).c_str());
    }
});

void PrintChat(const char *fmt, ...)
{
#if ENABLE_VISUALS
    CHudBaseChat *chat = (CHudBaseChat *) g_CHUD->FindElement("CHudChat");
    if (chat)
    {
        std::unique_ptr<char[]> buf(new char[1024]);
        va_list list;
        va_start(list, fmt);
        vsprintf(buf.get(), fmt, list);
        va_end(list);
        std::unique_ptr<char[]> str = std::move(strfmt("\x07%06X[\x07%06XCAT\x07%06X]\x01 %s", 0x5e3252, 0xba3d9a, 0x5e3252, buf.get()));
        // FIXME DEBUG LOG
        logging::Info("%s", str.get());
        chat->Printf(str.get());
    }
#endif
}

// Get the point Your shots originate from
Vector getShootPos(Vector angle)
{
    Vector eye = g_pLocalPlayer->v_Eye;
    if (g_pLocalPlayer->weapon_mode != weapon_projectile || CE_BAD(LOCAL_W))
        return eye;

    Vector forward, right, up;
    AngleVectors3(VectorToQAngle(angle), &forward, &right, &up);

    std::optional<Vector> vecOffset(0.0f);
    switch (LOCAL_W->m_iClassID())
    {
    // Rocket launchers and flare guns/Pomson
    case CL_CLASS(CTFRocketLauncher):
    case CL_CLASS(CTFRocketLauncher_Mortar):
    case CL_CLASS(CTFRocketLauncher_AirStrike):
    case CL_CLASS(CTFRocketLauncher_DirectHit):
    case CL_CLASS(CTFFlareGun):
    case CL_CLASS(CTFFlareGun_Revenge):
    case CL_CLASS(CTFDRGPomson):
        // The original shoots centered, rest doesn't
        if (CE_INT(LOCAL_W, netvar.iItemDefinitionIndex) != 513)
        {
            vecOffset = Vector(23.5f, 12.0f, -3.0f);
            // Ducking changes offset
            if (CE_INT(LOCAL_E, netvar.iFlags) & FL_DUCKING)
                vecOffset->z = 8.0f;
        }
        break;

    // Pill/Pipebomb launchers
    case CL_CLASS(CTFPipebombLauncher):
    case CL_CLASS(CTFGrenadeLauncher):
    case CL_CLASS(CTFCannon):
        vecOffset = Vector(16.0f, 8.0f, -6.0f);
        break;

    case CL_CLASS(CTFSyringeGun):
        vecOffset = Vector(16.0f, 6.0f, -8.0f);
        break;

        // Huntsman
        /*case CL_CLASS(CTFCompoundBow):
            vecOffset = Vector(23.5f, -8.0f, -3.0f);
            break;*/

    default:
        break;
    }

    // We have an offset for the weapon that may or may not need to be applied
    if (vecOffset)
    {

        // Game checks 2000 HU infront of eye for a hit
        static const float distance = 2000.0f;

        Vector endpos = eye + (forward * distance);

        trace_t tr;
        Ray_t ray;

        trace::filter_default.SetSelf(RAW_ENT(g_pLocalPlayer->entity));
        ray.Init(eye, endpos);
        if (!tcm || g_Settings.is_create_move)
            g_ITrace->TraceRay(ray, MASK_SOLID, &trace::filter_default, &tr);

        // Replicate game behaviour, only use the offset if our trace has a big enough fraction
        if (tr.fraction <= 0.1)
        {
            // Flipped viewmodels flip the y
            if (re::C_TFWeaponBase::IsViewModelFlipped(RAW_ENT(LOCAL_W)))
                vecOffset->y *= -1.0f;
            eye = eye + (forward * vecOffset->x) + (right * vecOffset->y) + (up * vecOffset->z);
            // They decided to do this weird stuff for the pomson instead of fixing their offset
            if (LOCAL_W->m_iClassID() == CL_CLASS(CTFDRGPomson))
                eye.z -= 13.0f;
        }
    }

    return eye;
}

// You shouldn't delete[] this unique_ptr since it
// does it on its own
std::unique_ptr<char[]> strfmt(const char *fmt, ...)
{
    // char *buf = new char[1024];
    auto buf = std::make_unique<char[]>(1024);
    va_list list;
    va_start(list, fmt);
    vsprintf(buf.get(), fmt, list);
    va_end(list);
    return buf;
}

void ChangeName(std::string name)
{
    auto custom_name = settings::Manager::instance().lookup("name.custom");
    if (custom_name != nullptr)
        custom_name->fromString(name);

    ReplaceSpecials(name);
    NET_SetConVar setname("name", name.c_str());
    INetChannel *ch = (INetChannel *) g_IEngine->GetNetChannelInfo();
    if (ch)
    {
        setname.SetNetChannel(ch);
        setname.SetReliable(false);
        ch->SendNetMsg(setname, false);
    }
}
const char *powerups[] = { "Strength", "Resistance", "Vampire", "Reflect", "Haste", "Regeneration", "Precision", "Agility", "Knockout", "King", "Plague", "Supernova", "Revenge" };

const std::string classes[] = { "Scout", "Sniper", "Soldier", "Demoman", "Medic", "Heavy", "Pyro", "Spy", "Engineer" };

// This and below taken from leaks
static int SeedFileLineHash(int seedvalue, const char *sharedname, int additionalSeed)
{
    CRC32_t retval;

    CRC32_Init(&retval);

    CRC32_ProcessBuffer(&retval, (void *) &seedvalue, sizeof(int));
    CRC32_ProcessBuffer(&retval, (void *) &additionalSeed, sizeof(int));
    CRC32_ProcessBuffer(&retval, (void *) sharedname, Q_strlen(sharedname));

    CRC32_Final(&retval);

    return (int) (retval);
}

int SharedRandomInt(unsigned iseed, const char *sharedname, int iMinVal, int iMaxVal, int additionalSeed /*=0*/)
{
    int seed = SeedFileLineHash(iseed, sharedname, additionalSeed);
    g_pUniformStream->SetSeed(seed);
    return g_pUniformStream->RandomInt(iMinVal, iMaxVal);
}

bool HookNetvar(std::vector<std::string> path, ProxyFnHook &hook, RecvVarProxyFn function)
{
    auto pClass = g_IBaseClient->GetAllClasses();
    if (path.size() < 2)
        return false;
    while (pClass)
    {
        // Main class found
        if (!strcmp(pClass->m_pRecvTable->m_pNetTableName, path[0].c_str()))
        {
            RecvTable *curr_table = pClass->m_pRecvTable;
            for (size_t i = 1; i < path.size(); i++)
            {
                bool found = false;
                for (int j = 0; j < curr_table->m_nProps; j++)
                {
                    RecvPropRedef *pProp = (RecvPropRedef *) &(curr_table->m_pProps[j]);
                    if (!pProp)
                        continue;
                    if (!strcmp(path[i].c_str(), pProp->m_pVarName))
                    {
                        // Detect last iteration
                        if (i == path.size() - 1)
                        {
                            hook.init(pProp);
                            hook.setHook(function);
                            return true;
                        }
                        curr_table = pProp->m_pDataTable;
                        found      = true;
                    }
                }
                // We tried searching the netvar but found nothing
                if (!found)
                {
                    std::string full_path;
                    for (auto &s : path)
                        full_path += s + "";
                    logging::Info("Hooking netvar with path \"%s\" failed. Required member not found.");
                    return false;
                }
            }
        }
        pClass = pClass->m_pNext;
    }
    return false;
}

static bool is_truce_active = false;
bool isTruce()
{
    return is_truce_active;
}
void setTruce(bool status)
{
    is_truce_active = status;
}
