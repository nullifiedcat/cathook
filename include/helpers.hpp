/*
 * helpers.h
 *
 *  Created on: Oct 8, 2016
 *      Author: nullifiedcat
 */

#pragma once

class CachedEntity;
class IClientEntity;
class ConVar;
class ConCommand;
class CUserCmd;
class CCommand;
struct player_info_s;
class Vector;

class ICvar;
void SetCVarInterface(ICvar *iface);

constexpr float PI    = 3.14159265358979323846f;
constexpr float RADPI = 57.295779513082f;
//#define DEG2RAD(x) (float)(x) * (float)(PI / 180.0f)

#include <enums.hpp>
#include <conditions.hpp>
#include <entitycache.hpp>
#include <core/logging.hpp>

#include <string>
#include <sstream>
#include <vector>
#include <mutex>
#include <random>

#include <core/sdk.hpp>
#include "sdk/dt_recv_redef.h"
#include "Ragdolls.hpp"

#define TICK_INTERVAL (g_GlobalVars->interval_per_tick)
#define TIME_TO_TICKS(dt) ((int) (0.5f + (float) (dt) / TICK_INTERVAL))
#define TICKS_TO_TIME(t) (TICK_INTERVAL * (t))
#define ROUND_TO_TICKS(t) (TICK_INTERVAL * TIME_TO_TICKS(t))

// typedef void ( *FnCommandCallback_t )( const CCommand &command );

template <typename Iter, typename RandomGenerator> Iter select_randomly(Iter start, Iter end, RandomGenerator &g)
{
    std::uniform_int_distribution<> dis(0, std::distance(start, end) - 1);
    std::advance(start, dis(g));
    return start;
}

template <typename Iter> Iter select_randomly(Iter start, Iter end)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    return select_randomly(start, end, gen);
}

// TODO split this shit

std::vector<ConVar *> &RegisteredVarsList();
std::vector<ConCommand *> &RegisteredCommandsList();

void BeginConVars();
void EndConVars();

// Calling source engine functions would get me more accurate result at cost of
// speed, so idk.
// TODO?

bool IsPlayerInvulnerable(CachedEntity *player);
bool IsPlayerCritBoosted(CachedEntity *player);
bool IsPlayerInvisible(CachedEntity *player);
bool IsPlayerDisguised(CachedEntity *player);
bool IsPlayerResistantToCurrentWeapon(CachedEntity *player);

const char *GetBuildingName(CachedEntity *ent);
Vector GetBuildingPosition(CachedEntity *ent);
bool IsBuildingVisible(CachedEntity *ent);

ConVar *CreateConVar(std::string name, std::string value, std::string help);
ConCommand *CreateConCommand(const char *name, FnCommandCallback_t callback, const char *help);

powerup_type GetPowerupOnPlayer(CachedEntity *player);
// GetHitbox() is being called really frequently.
// It's better if it won't create a new object each time it gets called.
// So it returns a success state, and the values are stored in out reference.
bool GetHitbox(CachedEntity *entity, int hb, Vector &out);
weaponmode GetWeaponMode();
weaponmode GetWeaponMode(CachedEntity *ent);

void FixMovement(CUserCmd &cmd, Vector &viewangles);
void VectorAngles(Vector &forward, Vector &angles);
void AngleVectors2(const QAngle &angles, Vector *forward);
void AngleVectors3(const QAngle &angles, Vector *forward, Vector *right, Vector *up);
bool isRapidFire(IClientEntity *wep);
extern std::mutex trace_lock;
bool IsEntityVisible(CachedEntity *entity, int hb);
bool IsEntityVectorVisible(CachedEntity *entity, Vector endpos, bool use_weapon_offset = false, unsigned int mask = MASK_SHOT_HULL, trace_t *trace = nullptr);
bool VisCheckEntFromEnt(CachedEntity *startEnt, CachedEntity *endEnt);
bool VisCheckEntFromEntVector(Vector startVector, CachedEntity *startEnt, CachedEntity *endEnt);
Vector VischeckCorner(CachedEntity *player, CachedEntity *target, float maxdist, bool checkWalkable);
std::pair<Vector, Vector> VischeckWall(CachedEntity *player, CachedEntity *target, float maxdist, bool checkWalkable);
float vectorMax(Vector i);
Vector vectorAbs(Vector i);
bool canReachVector(Vector loc, Vector dest = { 0, 0, 0 });

bool LineIntersectsBox(Vector &bmin, Vector &bmax, Vector &lmin, Vector &lmax);
void GenerateBoxVertices(const Vector &vOrigin, const QAngle &angles, const Vector &vMins, const Vector &vMaxs, Vector pVerts[8]);

float DistToSqr(CachedEntity *entity);
void fClampAngle(Vector &qaAng);
// const char* MakeInfoString(IClientEntity* player);
bool GetProjectileData(CachedEntity *weapon, float &speed, float &gravity, float &start_velocity);
bool IsVectorVisible(Vector a, Vector b, bool enviroment_only = false, CachedEntity *self = LOCAL_E, unsigned int mask = MASK_SHOT_HULL);
// A Special function for navparser to check if a Vector is visible.
bool IsVectorVisibleNavigation(Vector a, Vector b, unsigned int mask = MASK_SHOT_HULL);
Vector getShootPos(Vector angle);
Vector GetForwardVector(Vector origin, Vector viewangles, float distance, CachedEntity *punch_entity = nullptr);
Vector GetForwardVector(float distance, CachedEntity *punch_entity = nullptr);
CachedEntity *getClosestEntity(Vector vec);
CachedEntity *getClosestNonlocalEntity(Vector vec);
bool IsSentryBuster(CachedEntity *ent);
std::unique_ptr<char[]> strfmt(const char *fmt, ...);
// TODO move that to weaponid.h
int getWeaponByID(CachedEntity *player, int weaponid);
bool HasWeapon(CachedEntity *ent, int wantedId);
bool IsAmbassador(CachedEntity *ent);
bool AmbassadorCanHeadshot();
// Validate a UserCmd
#define VERIFIED_CMD_SIZE 90
void ValidateUserCmd(CUserCmd *cmd, int sequence_nr);

// Convert a TF2 handle into an IDX -> ENTITY(IDX)
int HandleToIDX(int handle);

inline const char *teamname(int team)
{
    if (team == 2)
        return "RED";
    else if (team == 3)
        return "BLU";
    return "SPEC";
}
extern const std::string classes[10];
inline const char *classname(int clazz)
{
    if (clazz > 0 && clazz < 10)
    {
        return classes[clazz - 1].c_str();
    }
    return "Unknown";
}

void PrintChat(const char *fmt, ...);
void ChangeName(std::string name);

void WhatIAmLookingAt(int *result_eindex, Vector *result_pos);

inline Vector GetAimAtAngles(Vector origin, Vector target, CachedEntity *punch_correct = nullptr)
{
    Vector angles, tr;
    tr = (target - origin);
    VectorAngles(tr, angles);
    // Apply punchangle correction
    if (punch_correct)
        angles -= CE_VECTOR(punch_correct, netvar.vecPunchAngle);
    fClampAngle(angles);
    return angles;
}

void AimAt(Vector origin, Vector target, CUserCmd *cmd, bool compensate_punch = true);
void FastStop();
void AimAtHitbox(CachedEntity *ent, int hitbox, CUserCmd *cmd, bool compensate_punch = true);
bool IsProjectileCrit(CachedEntity *ent);

QAngle VectorToQAngle(Vector in);
Vector QAngleToVector(QAngle in);

bool CanHeadshot();
bool CanShoot();

char GetUpperChar(ButtonCode_t button);
char GetChar(ButtonCode_t button);

bool IsEntityVisiblePenetration(CachedEntity *entity, int hb);

// void RunEnginePrediction(IClientEntity* ent, CUserCmd *ucmd = NULL);
// void StartPrediction(CUserCmd* cmd);
// void EndPrediction();

float RandFloatRange(float min, float max);
int UniformRandomInt(int min, int max);

Vector CalcAngle(Vector src, Vector dst);
void MakeVector(Vector ang, Vector &out);
float GetFov(Vector ang, Vector src, Vector dst);

void ReplaceString(std::string &input, const std::string &what, const std::string &with_what);
void ReplaceSpecials(std::string &input);

Vector ComputeMove(const Vector &a, const Vector &b);
std::pair<float, float> ComputeMovePrecise(const Vector &a, const Vector &b);
void WalkTo(const Vector &vector);

std::string GetLevelName();

int SharedRandomInt(unsigned iseed, const char *sharedname, int iMinVal, int iMaxVal, int additionalSeed /*=0*/);
bool HookNetvar(std::vector<std::string> path, ProxyFnHook &hook, RecvVarProxyFn function);
float ATTRIB_HOOK_FLOAT(float base_value, const char *search_string, IClientEntity *ent, void *buffer, bool is_global_const_string);

void format_internal(std::stringstream &stream);
template <typename T, typename... Targs> void format_internal(std::stringstream &stream, T value, Targs... args)
{
    stream << value;
    format_internal(stream, args...);
}
template <typename... Args> std::string format(const Args &...args)
{
    std::stringstream stream;
    format_internal(stream, args...);
    return stream.str();
}

extern const std::string classes[10];
extern const char *powerups[POWERUP_COUNT];
bool isTruce();
void setTruce(bool status);
