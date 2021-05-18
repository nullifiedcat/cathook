#include "common.hpp"
#include "DetourHook.hpp"

namespace hacks::tf2::animfix
{
static settings::Boolean enabled("misc.animfix.enabled", "false");
DetourHook frameadvance_detour{};
DetourHook shouldinterpolate_detour{};
typedef float (*FrameAdvance_t)(IClientEntity *, float);
typedef bool (*ShouldInterpolate_t)(IClientEntity *);
// typedef void (*CheckForSequenceChange_t)(int *, int *, int, bool, bool);

std::vector<float> previous_simtimes;

// Credits to Blackfire62 for telling me how this could be realized
float FrameAdvance_hook(IClientEntity *self, float flInterval)
{
    float newInterval = flInterval;

    // Check if the entity is valid
    if (enabled && self && IDX_GOOD(self->entindex()) && self->entindex() > 0 && self->entindex() <= (int) previous_simtimes.size())
    {
        // Check if they are an alive player
        CachedEntity *ent = ENTITY(self->entindex());
        // Set new interval based on their simtime
        if (CE_GOOD(ent) && ent->m_Type() == ENTITY_PLAYER && ent->m_bAlivePlayer())
        {
            float simtime = CE_FLOAT(ent, netvar.m_flSimulationTime);
            // Calculate the time we need to animate by
            float time_difference = simtime - previous_simtimes.at(ent->m_IDX - 1);
            if (time_difference > 0.0f)
                newInterval = time_difference;
            previous_simtimes.at(ent->m_IDX - 1) = simtime;
            // If the simtime didn't update we need to make sure that the original function also does not update
            if (newInterval == 0.0f)
                CE_FLOAT(ent, netvar.m_flAnimTime) = g_GlobalVars->curtime;
        }
    }

    FrameAdvance_t original = (FrameAdvance_t) frameadvance_detour.GetOriginalFunc();
    float return_value      = original(self, newInterval);
    frameadvance_detour.RestorePatch();
    return return_value;
}

bool ShouldInterpolate_hook(IClientEntity *ent)
{
    if (enabled)
    {
        if (ent && IDX_GOOD(ent->entindex()))
        {
            CachedEntity *cent = ENTITY(ent->entindex());
            if (cent->m_Type() == ENTITY_PLAYER && cent->m_IDX != g_pLocalPlayer->entity_idx)
            {
                return false;
            }
        }
    }
    ShouldInterpolate_t original = (ShouldInterpolate_t) shouldinterpolate_detour.GetOriginalFunc();
    bool ret                     = original(ent);
    shouldinterpolate_detour.RestorePatch();
    return ret;
}

// We need a non crashing way to implement this. Currently it will just cause crashes
// due to race conditions That we cannot resolve
/*std::mutex threadsafe_mutex;

void CheckForSequenceChange_hook(int *_this, int *studiohdr, int sequence, bool forcenewsequence, bool bInterpolate)
{
    bInterpolate       = false;
    auto new_studiohdr = studiohdr;
    if (enabled)
    {
        new_studiohdr = nullptr;
    }

    std::lock_guard<std::mutex> checkforsequencechance_mutex(threadsafe_mutex);
    CheckForSequenceChange_t original = (CheckForSequenceChange_t) checkforsequencechange_detour.GetOriginalFunc();
    original(_this, new_studiohdr, sequence, forcenewsequence, bInterpolate);
    checkforsequencechange_detour.RestorePatch();
}*/

void LevelInit()
{
    previous_simtimes.clear();
    previous_simtimes.resize(g_IEngine->GetMaxClients());
}

static InitRoutine init([]() {
    static auto ShouldInterpolate_signature = gSignatures.GetClientSignature("55 89 E5 53 83 EC 14 A1 ? ? ? ? 8B 5D ? 8B 10 89 04 24 FF 52 ? 8B 53");
    shouldinterpolate_detour.Init(ShouldInterpolate_signature, (void *) ShouldInterpolate_hook);

    /*static auto CheckForSequenceChange_signature = gSignatures.GetClientSignature("55 89 E5 57 56 53 83 EC 1C 8B 45 ? 8B 75 ? 8B 4D ? 8B 7D");
     checkforsequencechange_detour.Init(CheckForSequenceChange_signature, (void *) CheckForSequenceChange_hook);*/

    static auto FrameAdvance_signature = gSignatures.GetClientSignature("55 89 E5 57 56 53 83 EC 4C 8B 5D ? 80 BB ? ? ? ? 00 0F 85 ? ? ? ? 8B B3");
    frameadvance_detour.Init(FrameAdvance_signature, (void *) FrameAdvance_hook);
    EC::Register(EC::LevelInit, LevelInit, "levelinit_animfix");
    LevelInit();

    EC::Register(
        EC::Shutdown,
        []() {
            frameadvance_detour.Shutdown();
            // checkforsequencechange_detour.Shutdown();
            frameadvance_detour.Shutdown();
        },
        "shutdown_animfix");
});
} // namespace hacks::tf2::animfix
