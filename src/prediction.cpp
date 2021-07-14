/*
 * prediction.cpp
 *
 *  Created on: Dec 5, 2016
 *      Author: nullifiedcat
 */
#include "common.hpp"
#include <settings/Bool.hpp>
#include <boost/circular_buffer.hpp>

static settings::Boolean debug_pp_extrapolate{ "debug.pp-extrapolate", "false" };
static settings::Boolean debug_pp_draw{ "debug.pp-draw", "false" };
static settings::Boolean debug_pp_bsearch{ "debug.pp-bsearch", "false" };
static settings::Boolean debug_pp_gravity{ "debug.pp-gravity", "false" };
// The higher the sample size, the more previous positions we will take into account to calculate the next position. Lower = Faster reaction Higher = Stability
static settings::Int sample_size("debug.strafepred.samplesize", "10");
// TODO there is a Vector() object created each call.

Vector SimpleLatencyPrediction(CachedEntity *ent, int hb)
{
    if (!ent)
        return Vector();
    Vector result;
    GetHitbox(ent, hb, result);
    float latency = g_IEngine->GetNetChannelInfo()->GetLatency(FLOW_OUTGOING) + g_IEngine->GetNetChannelInfo()->GetLatency(FLOW_INCOMING);
    Vector velocity;
    velocity::EstimateAbsVelocity(RAW_ENT(ent), velocity);
    result += velocity * latency;
    return result;
}

float PlayerGravityMod(CachedEntity *player)
{
    //      int movetype = CE_INT(player, netvar.movetype);
    //      if (movetype == MOVETYPE_FLY || movetype == MOVETYPE_NOCLIP) return
    //      0.0f;
    if (HasCondition<TFCond_Parachute>(player))
        return 0.448f;
    return 1.0f;
}

struct StrafePredictionData
{
    bool strafing_dir;
    float radius;
    Vector center;
};

// StrafePredictionData strafepred_data;
static std::array<boost::circular_buffer<Vector>, MAX_PLAYERS> previous_positions;
static ConVar *sv_gravity = nullptr;

// Function for calculating the center and radius of a circle that is supposed to represent a players starfing pattern
static std::optional<StrafePredictionData> findCircle(const Vector &current, const Vector &past1, const Vector &past2)
{
    float yDelta_a = past1.y - current.y;
    float xDelta_a = past1.x - current.x;
    float yDelta_b = past2.y - past1.y;
    float xDelta_b = past2.x - past1.x;
    Vector2D Center;

    float aSlope = yDelta_a / xDelta_a;
    float bSlope = yDelta_b / xDelta_b;

    Vector2D AB_Mid = Vector2D((current.x + past1.x) / 2, (current.y + past1.y) / 2);
    Vector2D BC_Mid = Vector2D((past1.x + past2.x) / 2, (past1.y + past2.y) / 2);

    if (yDelta_a == 0)
    {
        Center.x = AB_Mid.x;

        if (xDelta_b == 0)
            return std::nullopt;
        else
        {
            Center.y = BC_Mid.y + (BC_Mid.x - Center.x) / bSlope;
        }
    }
    else if (yDelta_b == 0)
    {
        Center.x = BC_Mid.x;
        if (xDelta_a == 0)
            return std::nullopt;
        else
            Center.y = AB_Mid.y + (AB_Mid.x - Center.x) / aSlope;
    }
    else if (xDelta_a == 0)
        return std::nullopt;
    else if (xDelta_b == 0)
        return std::nullopt;
    else
    {
        Center.x = (aSlope * bSlope * (AB_Mid.y - BC_Mid.y) - aSlope * BC_Mid.x + bSlope * AB_Mid.x) / (bSlope - aSlope);
        Center.y = AB_Mid.y - (Center.x - AB_Mid.x) / aSlope;
    }

    float Radius = current.AsVector2D().DistTo(Center);

    if (Radius < 5.0f || Radius > 500.0f)
        return std::nullopt;

    float Angle         = atan2(current.y - Center.y, current.x - Center.x);
    float PreviousAngle = atan2(past1.y - Center.y, past1.x - Center.x);

    Vector Center3D(Center.x, Center.y, current.z);

    return StrafePredictionData{ Angle < PreviousAngle, Radius, Center3D };
}

// Applies strafe predictions to the next position
void applyStrafePrediction(Vector &pos, StrafePredictionData &data, float prediction_distance)
{
    // Calculate our current angle on the circle
    float curangle = atan2(pos.y - data.center.y, pos.x - data.center.x);

    // Ok so here we calculate the angle based on the distance the normal prediction threw out, stored in prediction_distance
    // central angle = arclength / radius
    float newangle = prediction_distance / data.radius;

    // We then use that angle to calculate the actual 2d position and insert that into PosOnCircle
    Vector PosOnCircle;
    if (!data.strafing_dir)
        PosOnCircle = Vector(data.center.x + data.radius * cos(curangle + newangle), data.center.y + data.radius * sin(curangle + newangle), 0);
    else
        PosOnCircle = Vector(data.center.x + data.radius * cos(curangle - newangle), data.center.y + data.radius * sin(curangle - newangle), 0);

    pos.x = PosOnCircle.x;
    pos.y = PosOnCircle.y;
}

// Check if this target is eligible for strafe prediction and return an object containing info if it is
std::optional<StrafePredictionData> initializeStrafePrediction(CachedEntity *ent)
{
    if (g_pLocalPlayer->weapon_mode == weapon_projectile && ent->m_Type() == ENTITY_PLAYER && ent->m_IDX > 0 && ent->m_IDX <= g_GlobalVars->maxClients && !(CE_INT(ent, netvar.iFlags) & FL_ONGROUND))
    {
        auto &buffer = previous_positions.at(ent->m_IDX - 1);
        if (buffer.full())
            return findCircle(buffer.front(), buffer.at(buffer.capacity() / 2), buffer.back());
    }
    return std::nullopt;
}

Vector PredictStep(Vector pos, Vector &vel, const Vector &acceleration, std::pair<Vector, Vector> *minmax, float steplength, StrafePredictionData *strafepred, bool vischeck, std::optional<float> grounddistance)
{
    PROF_SECTION(PredictNew)
    Vector result = pos;

    // If we should do strafe prediction, then we still need to do the calculations, but instead of applying them we simply calculate the distance traveled and use that info together with strafe pred
    if (strafepred && (grounddistance ? *grounddistance > 0.1f : DistanceToGround(pos, minmax->first, minmax->second) > 0.1f))
    {
        auto newpos = result + (acceleration / 2.0f) * pow(steplength, 2) + vel * steplength;
        // Strafe pred does not predict Z! The player can't control his Z anyway, so it is pointless.
        result.z = newpos.z;
        applyStrafePrediction(result, *strafepred, newpos.DistTo(result));
    }
    else
        result += (acceleration / 2.0f) * pow(steplength, 2) + vel * steplength;

    vel += acceleration * steplength;

    if (vischeck)
    {
        Vector modify = result;
        // Standing Player height is 83
        modify.z   = pos.z + 83.0f;
        Vector low = modify;
        low.z -= 8912.0f;

        {
            PROF_SECTION(PredictTraces)
            Ray_t ray;
            trace_t trace;
            if (minmax)
                ray.Init(modify, low, minmax->first, minmax->second);
            else
                ray.Init(modify, low);
            g_ITrace->TraceRay(ray, MASK_PLAYERSOLID, &trace::filter_no_player, &trace);

            float dist = pos.z - trace.endpos.z;
            if (trace.m_pEnt && std::fabs(dist) < 63.0f)
                grounddistance = dist;
        }
    }
    if (grounddistance)
        if (result.z < pos.z - *grounddistance)
            result.z = pos.z - *grounddistance;
    return result;
}

std::vector<Vector> Predict(CachedEntity *player, Vector pos, float offset, Vector vel, Vector acceleration, std::pair<Vector, Vector> minmax, int count, bool vischeck)
{
    std::vector<Vector> positions;
    positions.reserve(count);

    pos.z -= offset;

    float dist       = DistanceToGround(pos, minmax.first, minmax.second);
    auto strafe_pred = initializeStrafePrediction(player);

    for (int i = 0; i < count; i++)
    {
        if (vischeck)
            pos = PredictStep(pos, vel, acceleration, &minmax, g_GlobalVars->interval_per_tick, strafe_pred ? &*strafe_pred : nullptr);
        else
            pos = PredictStep(pos, vel, acceleration, &minmax, g_GlobalVars->interval_per_tick, strafe_pred ? &*strafe_pred : nullptr, false, dist);
        positions.push_back({ pos.x, pos.y, pos.z + offset });
    }
    return positions;
}

#if ENABLE_VISUALS
void Prediction_PaintTraverse()
{
    if (g_Settings.bInvalid)
        return;
    if (debug_pp_draw)
    {
        if (!sv_gravity)
        {
            sv_gravity = g_ICvar->FindVar("sv_gravity");
            if (!sv_gravity)
                return;
        }
        for (int i = 1; i < g_GlobalVars->maxClients; i++)
        {
            auto ent = ENTITY(i);
            if (CE_BAD(ent) || !ent->m_bAlivePlayer())
                continue;

            Vector velocity;
            velocity::EstimateAbsVelocity(RAW_ENT(ent), velocity);
            auto data = Predict(ent, ent->m_vecOrigin(), 0.0f, velocity, Vector(0, 0, -sv_gravity->GetFloat()), std::make_pair(RAW_ENT(ent)->GetCollideable()->OBBMins(), RAW_ENT(ent)->GetCollideable()->OBBMaxs()), 64);
            Vector previous_screen;
            if (!draw::WorldToScreen(ent->m_vecOrigin(), previous_screen))
                continue;
            rgba_t color = colors::FromRGBA8(255, 0, 0, 255);
            for (size_t j = 0; j < data.size(); j++)
            {
                Vector screen;
                if (draw::WorldToScreen(data[j], screen))
                {
                    draw::Line(screen.x, screen.y, previous_screen.x - screen.x, previous_screen.y - screen.y, color, 2);
                    previous_screen = screen;
                }
                else
                {
                    break;
                }
                color.r -= 1.0f / 20.0f;
            }

            /*if (!ent->m_bEnemy())
                continue;
            auto pos = ProjectilePrediction(ent, hitbox_t::spine_3, 1980.0f, 0.0f, 1.0f, 0.0f);

            Vector aaa;
            if (draw::WorldToScreen(pos, aaa))
                draw::Rectangle(aaa.x, aaa.y, 5, 5, colors::yellow);*/
        }
    }
}
#endif
Vector EnginePrediction(CachedEntity *entity, float time)
{
    Vector result      = entity->m_vecOrigin();
    IClientEntity *ent = RAW_ENT(entity);

    typedef void (*SetupMoveFn)(IPrediction *, IClientEntity *, CUserCmd *, class IMoveHelper *, CMoveData *);
    typedef void (*FinishMoveFn)(IPrediction *, IClientEntity *, CUserCmd *, CMoveData *);

    void **predictionVtable  = *((void ***) g_IPrediction);
    SetupMoveFn oSetupMove   = (SetupMoveFn) (*(unsigned *) (predictionVtable + 19));
    FinishMoveFn oFinishMove = (FinishMoveFn) (*(unsigned *) (predictionVtable + 20));

    // CMoveData *pMoveData = (CMoveData*)(sharedobj::client->lmap->l_addr +
    // 0x1F69C0C);  CMoveData movedata {};
    auto pMoveData = std::make_unique<CMoveData>();

    float frameTime = g_GlobalVars->frametime;
    float curTime   = g_GlobalVars->curtime;

    CUserCmd fakecmd{};

    Vector vel;
    velocity::EstimateAbsVelocity(RAW_ENT(entity), vel);
    fakecmd.command_number = last_cmd_number;
    fakecmd.forwardmove    = vel.x;
    fakecmd.sidemove       = -vel.y;
    Vector oldangles       = CE_VECTOR(entity, netvar.m_angEyeAngles);
    static Vector zerov{ 0, 0, 0 };
    CE_VECTOR(entity, netvar.m_angEyeAngles) = zerov;

    CUserCmd *original_cmd = NET_VAR(ent, 4188, CUserCmd *);

    NET_VAR(ent, 4188, CUserCmd *) = &fakecmd;

    g_GlobalVars->curtime   = g_GlobalVars->interval_per_tick * NET_INT(ent, netvar.nTickBase);
    g_GlobalVars->frametime = time;

    Vector old_origin      = entity->m_vecOrigin();
    NET_VECTOR(ent, 0x354) = entity->m_vecOrigin();

    *g_PredictionRandomSeed = MD5_PseudoRandom(current_user_cmd->command_number) & 0x7FFFFFFF;
    g_IGameMovement->StartTrackPredictionErrors(reinterpret_cast<CBasePlayer *>(ent));
    oSetupMove(g_IPrediction, ent, &fakecmd, nullptr, pMoveData.get());
    g_IGameMovement->ProcessMovement(reinterpret_cast<CBasePlayer *>(ent), pMoveData.get());
    oFinishMove(g_IPrediction, ent, &fakecmd, pMoveData.get());
    g_IGameMovement->FinishTrackPredictionErrors(reinterpret_cast<CBasePlayer *>(ent));

    NET_VAR(entity, 4188, CUserCmd *) = original_cmd;

    g_GlobalVars->frametime = frameTime;
    g_GlobalVars->curtime   = curTime;

    result                                    = ent->GetAbsOrigin();
    NET_VECTOR(ent, 0x354)                    = old_origin;
    CE_VECTOR(entity, netvar.m_angEyeAngles)  = oldangles;
    const_cast<Vector &>(ent->GetAbsOrigin()) = old_origin;
    const_cast<QAngle &>(ent->GetAbsAngles()) = VectorToQAngle(oldangles);

    return result;
}

std::pair<Vector, Vector> ProjectilePrediction_Engine(CachedEntity *ent, int hb, float speed, float gravitymod, float entgmod /* ignored */, float proj_startvelocity)
{
    Vector origin = ent->m_vecOrigin();
    Vector hitbox;
    GetHitbox(ent, hb, hitbox);
    Vector hitbox_offset = hitbox - origin;

    if (!sv_gravity)
        sv_gravity = g_ICvar->FindVar("sv_gravity");

    if (speed == 0.0f || !sv_gravity)
        return { Vector(), Vector() };

    // TODO ProjAim
    float medianTime  = g_pLocalPlayer->v_Eye.DistTo(hitbox) / speed;
    float range       = 1.5f;
    float currenttime = medianTime - range;
    if (currenttime <= 0.0f)
        currenttime = 0.01f;
    float besttime = currenttime;
    float mindelta = 65536.0f;
    Vector bestpos = origin;
    Vector current = origin;
    int maxsteps   = 40;
    bool onground  = false;
    if (ent->m_Type() == ENTITY_PLAYER)
    {
        if (CE_INT(ent, netvar.iFlags) & FL_ONGROUND)
            onground = true;
    }
    float steplength = ((float) (2 * range) / (float) maxsteps);
    Vector ent_mins  = RAW_ENT(ent)->GetCollideable()->OBBMins();
    Vector ent_maxs  = RAW_ENT(ent)->GetCollideable()->OBBMaxs();
    float grav_mul   = sv_gravity->GetFloat() / 2.0f * gravitymod;
    if (debug_pp_bsearch)
    {
        float mintime = currenttime, maxtime = currenttime + range * 2;

        for (int steps = 0; steps < maxsteps; steps++)
        {
            Vector minloc = EnginePrediction(ent, mintime);
            Vector maxloc = EnginePrediction(ent, maxtime);

            if (onground)
            {
                minloc.z -= DistanceToGround(minloc, ent_mins, ent_maxs);
                maxloc.z -= DistanceToGround(maxloc, ent_mins, ent_maxs);
            }

            if (debug_pp_gravity)
            {
                minloc.z += grav_mul * pow(mintime, 2);
                maxloc.z += grav_mul * pow(maxtime, 2);
            }

            const float minairtime = g_pLocalPlayer->v_Eye.DistTo(minloc) / speed;
            const float maxairtime = g_pLocalPlayer->v_Eye.DistTo(maxloc) / speed;

            const float mindelta = abs(minairtime - mintime);
            const float maxdelta = abs(maxairtime - maxtime);

            const float midtime = (mintime + maxtime) / 2.0f;

            if (mindelta < maxdelta)
            {
                maxtime  = midtime;
                besttime = mintime;
                bestpos  = minloc;
            }
            else
            {
                mintime  = midtime;
                besttime = maxtime;
                bestpos  = maxloc;
            }
        }
    }
    else
    {
        for (int steps = 0; steps < maxsteps; steps++, currenttime += steplength)
        {
            ent->m_vecOrigin() = current;
            current            = EnginePrediction(ent, steplength);

            if (onground)
            {
                float toground = DistanceToGround(current, ent_mins, ent_maxs);
                current.z -= toground;
            }

            if (debug_pp_gravity)
            {
                current.z += grav_mul * pow(currenttime, 2);
            }

            float rockettime = g_pLocalPlayer->v_Eye.DistTo(current) / speed;
            // Compensate for ping
            // rockettime += g_IEngine->GetNetChannelInfo()->GetLatency(FLOW_OUTGOING) + cl_interp->GetFloat();
            if (fabs(rockettime - currenttime) < mindelta)
            {
                besttime = currenttime;
                bestpos  = current;
                mindelta = fabs(rockettime - currenttime);
            }
        }
    }
    const_cast<Vector &>(RAW_ENT(ent)->GetAbsOrigin()) = origin;
    CE_VECTOR(ent, 0x354)                              = origin;
    // Compensate for ping
    // besttime += g_IEngine->GetNetChannelInfo()->GetLatency(FLOW_OUTGOING) + cl_interp->GetFloat();
    bestpos.z += (sv_gravity->GetFloat() / 2.0f * besttime * besttime * gravitymod);
    // S = at^2/2 ; t = sqrt(2S/a)*/
    Vector result            = bestpos + hitbox_offset;
    Vector result_initialvel = result;
    result_initialvel.z -= proj_startvelocity * besttime;
    /*logging::Info("[Pred][%d] delta: %.2f   %.2f   %.2f", result.x - origin.x,
                  result.y - origin.y, result.z - origin.z);*/
    return { result, result_initialvel };
}
std::pair<Vector, Vector> BuildingPrediction(CachedEntity *building, Vector vec, float speed, float gravity, float proj_startvelocity)
{
    if (!vec.z || CE_BAD(building))
        return { Vector(), Vector() };
    Vector result = vec;
    // if (not debug_pp_extrapolate) {
    //} else {
    //        result = SimpleLatencyPrediction(ent, hb);
    //
    //}

    if (!sv_gravity)
        sv_gravity = g_ICvar->FindVar("sv_gravity");

    if (speed == 0.0f || !sv_gravity)
        return { Vector(), Vector() };

    trace::filter_no_player.SetSelf(RAW_ENT(building));

    // TODO ProjAim
    float medianTime  = g_pLocalPlayer->v_Eye.DistTo(result) / speed;
    float range       = 1.5f;
    float currenttime = medianTime - range;
    if (currenttime <= 0.0f)
        currenttime = 0.01f;
    float besttime = currenttime;
    float mindelta = 65536.0f;
    Vector bestpos = result;
    int maxsteps   = 300;
    for (int steps = 0; steps < maxsteps; steps++, currenttime += ((float) (2 * range) / (float) maxsteps))
    {
        Vector curpos    = result;
        float rockettime = g_pLocalPlayer->v_Eye.DistTo(curpos) / speed;
        // Compensate for ping
        rockettime += g_IEngine->GetNetChannelInfo()->GetLatency(FLOW_OUTGOING) + cl_interp->GetFloat();
        if (fabs(rockettime - currenttime) < mindelta)
        {
            besttime = currenttime;
            bestpos  = curpos;
            mindelta = fabs(rockettime - currenttime);
        }
    }
    // Compensate for ping
    besttime += g_IEngine->GetNetChannelInfo()->GetLatency(FLOW_OUTGOING) + cl_interp->GetFloat();
    bestpos.z += (sv_gravity->GetFloat() / 2.0f * besttime * besttime * gravity);
    Vector bestpos_initialvel = bestpos;
    bestpos_initialvel.z -= proj_startvelocity * besttime;
    // S = at^2/2 ; t = sqrt(2S/a)*/
    return { bestpos, bestpos_initialvel };
}

std::pair<Vector, Vector> ProjectilePrediction(CachedEntity *ent, int hb, float speed, float gravitymod, float entgmod, float proj_startvelocity)
{
    Vector origin = ent->m_vecOrigin();
    Vector hitbox;
    GetHitbox(ent, hb, hitbox);
    Vector hitbox_offset = hitbox - origin;

    if (!sv_gravity)
        sv_gravity = g_ICvar->FindVar("sv_gravity");

    if (speed == 0.0f || !sv_gravity)
        return { Vector(), Vector() };

    // TODO ProjAim
    float medianTime  = g_pLocalPlayer->v_Eye.DistTo(hitbox) / speed;
    float range       = 1.5f;
    float currenttime = medianTime - range;
    if (currenttime <= 0.0f)
        currenttime = 0.01f;
    float besttime = currenttime;
    float mindelta = 65536.0f;
    Vector bestpos = origin;
    Vector current = origin;
    int maxsteps   = 40;
    bool onground  = false;
    if (ent->m_Type() == ENTITY_PLAYER)
    {
        if (CE_INT(ent, netvar.iFlags) & FL_ONGROUND)
            onground = true;
    }

    Vector velocity;
    velocity::EstimateAbsVelocity(RAW_ENT(ent), velocity);

    Vector acceleration = { 0.0f, 0.0f, -sv_gravity->GetFloat() * entgmod };
    float steplength    = ((float) (2 * range) / (float) maxsteps);
    auto minmax         = std::make_pair(RAW_ENT(ent)->GetCollideable()->OBBMins(), RAW_ENT(ent)->GetCollideable()->OBBMaxs());
    float grav_mul      = sv_gravity->GetFloat() / 2.0f * gravitymod;

    Vector last = origin;

    auto strafe_pred = initializeStrafePrediction(ent);

    if (debug_pp_bsearch)
    {
        Vector stepvel = velocity;
        float mintime = currenttime, maxtime = currenttime + range * 2;

        for (int steps = 0; steps < maxsteps; steps++)
        {
            const float latency = g_IEngine->GetNetChannelInfo()->GetLatency(FLOW_OUTGOING) + cl_interp->GetFloat();

            // Hack as PredictStep() updates velocity
            stepvel = velocity;
            Vector minloc = PredictStep(origin, velocity, acceleration, &minmax, mintime, strafe_pred ? &*strafe_pred : nullptr);
            stepvel = velocity;
            Vector maxloc = PredictStep(origin, velocity, acceleration, &minmax, maxtime, strafe_pred ? &*strafe_pred : nullptr);

            if (onground)
            {
                minloc.z -= DistanceToGround(minloc, minmax.first, minmax.second);
                maxloc.z -= DistanceToGround(maxloc, minmax.first, minmax.second);
            }

            if (debug_pp_gravity)
            {
                minloc.z += grav_mul * pow(mintime, 2);
                maxloc.z += grav_mul * pow(maxtime, 2);
            }

            const float minairtime = latency + g_pLocalPlayer->v_Eye.DistTo(minloc) / speed;
            const float maxairtime = latency + g_pLocalPlayer->v_Eye.DistTo(maxloc) / speed;

            const float mindelta = abs(minairtime - mintime);
            const float maxdelta = abs(maxairtime - maxtime);

            const float midtime = (mintime + maxtime) / 2.0f;

            if (mindelta < maxdelta)
            {
                maxtime  = midtime;
                besttime = mintime;
                bestpos  = minloc;
            }
            else
            {
                mintime  = midtime;
                besttime = maxtime;
                bestpos  = maxloc;
            }
        }
    }
    else
    {
        for (int steps = 0; steps < maxsteps; steps++, currenttime += steplength)
        {
            current = last = PredictStep(last, velocity, acceleration, &minmax, steplength, strafe_pred ? &*strafe_pred : nullptr);

            if (onground)
            {
                float toground = DistanceToGround(current, minmax.first, minmax.second);
                current.z -= toground;
            }

            float rockettime = g_pLocalPlayer->v_Eye.DistTo(current) / speed;
            // Compensate for ping
            rockettime += g_IEngine->GetNetChannelInfo()->GetLatency(FLOW_OUTGOING) + cl_interp->GetFloat();
            if (fabs(rockettime - currenttime) < mindelta)
            {
                besttime = currenttime;
                bestpos  = current;
                mindelta = fabs(rockettime - currenttime);
            }
        }
    }
    // Compensate for ping
    besttime += g_IEngine->GetNetChannelInfo()->GetLatency(FLOW_OUTGOING) + cl_interp->GetFloat();
    bestpos.z += (sv_gravity->GetFloat() / 2.0f * besttime * besttime * gravitymod);
    // S = at^2/2 ; t = sqrt(2S/a)*/
    Vector result            = bestpos + hitbox_offset;
    Vector initialvel_result = result;
    initialvel_result.z -= proj_startvelocity * besttime;
    /*logging::Info("[Pred][%d] delta: %.2f   %.2f   %.2f", result.x - origin.x,
                  result.y - origin.y, result.z - origin.z);*/
    return { result, initialvel_result };
}

float DistanceToGround(CachedEntity *ent)
{
    Vector origin = ent->m_vecOrigin();
    Vector mins   = RAW_ENT(ent)->GetCollideable()->OBBMins();
    Vector maxs   = RAW_ENT(ent)->GetCollideable()->OBBMins();
    return DistanceToGround(origin, mins, maxs);
}

float DistanceToGround(Vector origin, Vector mins, Vector maxs)
{
    trace_t ground_trace;
    Ray_t ray;
    Vector endpos = origin;
    endpos.z -= 8192;
    ray.Init(origin, endpos, mins, maxs);
    g_ITrace->TraceRay(ray, MASK_PLAYERSOLID, &trace::filter_no_player, &ground_trace);
    return std::fabs(origin.z - ground_trace.endpos.z);
}

float DistanceToGround(Vector origin)
{
    trace_t ground_trace;
    Ray_t ray;
    Vector endpos = origin;
    endpos.z -= 8192;
    ray.Init(origin, endpos);
    g_ITrace->TraceRay(ray, MASK_PLAYERSOLID, &trace::filter_no_player, &ground_trace);
    return std::fabs(origin.z - ground_trace.endpos.z);
}

static InitRoutine init(
    []()
    {
        previous_positions.fill(boost::circular_buffer<Vector>(*sample_size));
        sample_size.installChangeCallback([](settings::VariableBase<int> &, int after) { previous_positions.fill(boost::circular_buffer<Vector>(after)); });
        EC::Register(
            EC::CreateMove,
            []()
            {
                for (int i = 1; i < g_GlobalVars->maxClients; i++)
                {
                    auto ent     = ENTITY(i);
                    auto &buffer = previous_positions.at(i - 1);

                    if (CE_BAD(LOCAL_E) || CE_BAD(ent) || !ent->m_bAlivePlayer())
                    {
                        buffer.clear();
                        continue;
                    }
                    Vector vel;
                    velocity::EstimateAbsVelocity(RAW_ENT(ent), vel);
                    if (vel.IsZero())
                    {
                        buffer.clear();
                        continue;
                    }

                    buffer.push_front(ent->m_vecOrigin());
                }
            },
            "cm_prediction", EC::very_early);
    });
