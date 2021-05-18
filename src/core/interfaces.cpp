/*
 * interfaces.cpp
 *
 *  Created on: Oct 3, 2016
 *      Author: nullifiedcat
 */

#include "common.hpp"
#include "core/sharedobj.hpp"
#include <thread>

#include <unistd.h>

#include <string>
#include <sstream>

#include <steam/isteamclient.h>

// class ISteamFriends002;

IVModelRender *g_IVModelRender           = nullptr;
ISteamClient *g_ISteamClient             = nullptr;
ISteamFriends *g_ISteamFriends           = nullptr;
IVEngineClient013 *g_IEngine             = nullptr;
void *demoplayer                         = nullptr;
IEngineSound *g_ISoundEngine             = nullptr;
vgui::ISurface *g_ISurface               = nullptr;
vgui::IPanel *g_IPanel                   = nullptr;
vgui::ILocalize *g_ILocalize             = nullptr;
IClientEntityList *g_IEntityList         = nullptr;
ICvar *g_ICvar                           = nullptr;
IGameEventManager2 *g_IEventManager2     = nullptr;
IBaseClientDLL *g_IBaseClient            = nullptr;
IEngineTrace *g_ITrace                   = nullptr;
IVModelInfoClient *g_IModelInfo          = nullptr;
IInputSystem *g_IInputSystem             = nullptr;
CGlobalVarsBase **rg_GlobalVars          = nullptr;
IPrediction *g_IPrediction               = nullptr;
IGameMovement *g_IGameMovement           = nullptr;
IInput *g_IInput                         = nullptr;
ISteamUser *g_ISteamUser                 = nullptr;
IAchievementMgr *g_IAchievementMgr       = nullptr;
ISteamUserStats *g_ISteamUserStats       = nullptr;
IStudioRender *g_IStudioRender           = nullptr;
IVDebugOverlay *g_IVDebugOverlay         = nullptr;
IMaterialSystemFixed *g_IMaterialSystem  = nullptr;
IVRenderView *g_IVRenderView             = nullptr;
IMaterialSystem *g_IMaterialSystemHL     = nullptr;
IMoveHelperServer *g_IMoveHelperServer   = nullptr;
CBaseClientState *g_IBaseClientState     = nullptr;
IGameEventManager *g_IGameEventManager   = nullptr;
TFGCClientSystem *g_TFGCClientSystem     = nullptr;
CHud *g_CHUD                             = nullptr;
CGameRules *g_pGameRules                 = nullptr;
IEngineVGui *g_IEngineVGui               = nullptr;
IUniformRandomStream *g_pUniformStream   = nullptr;
int *g_PredictionRandomSeed              = nullptr;
IFileSystem *g_IFileSystem               = nullptr;
IMDLCache *g_IMDLCache                   = nullptr;
IToolFrameworkInternal *g_IToolFramework = nullptr;
CServerTools *g_IServerTools             = nullptr;

template <typename T> T *BruteforceInterface(std::string name, sharedobj::SharedObject &object, int start = 0)
{
    T *result = nullptr;
    std::stringstream stream;
    for (int i = start; i < 100; i++)
    {
        stream.str("");
        stream << name;
        int zeros = 0;
        if (i < 10)
            zeros = 2;
        else if (i < 100)
            zeros = 1;
        for (int j = 0; j < zeros; j++)
            stream << '0';
        stream << i;
        result = reinterpret_cast<T *>(object.CreateInterface(stream.str()));
        if (result)
            return result;
    }
    logging::Info("RIP Software: can't create interface %s!", name.c_str());
    exit(0);
    return nullptr;
}

extern "C" typedef HSteamPipe (*GetHSteamPipe_t)();
extern "C" typedef HSteamUser (*GetHSteamUser_t)();

void CreateEarlyInterfaces()
{
    g_IFileSystem = BruteforceInterface<IFileSystem>("VFileSystem", sharedobj::filesystem_stdio());
}

void CreateInterfaces()
{
    g_ICvar             = BruteforceInterface<ICvar>("VEngineCvar", sharedobj::vstdlib());
    g_IEngine           = BruteforceInterface<IVEngineClient013>("VEngineClient", sharedobj::engine());
    demoplayer          = **(unsigned ***) (gSignatures.GetEngineSignature("89 15 ? ? ? ? BA ? ? ? ? 83 38 01") + 2);
    g_ISoundEngine      = BruteforceInterface<IEngineSound>("IEngineSoundClient", sharedobj::engine());
    g_AppID             = g_IEngine->GetAppID();
    g_IEntityList       = BruteforceInterface<IClientEntityList>("VClientEntityList", sharedobj::client());
    g_ISteamClient      = BruteforceInterface<ISteamClient>("SteamClient", sharedobj::steamclient(), 17);
    g_IEventManager2    = BruteforceInterface<IGameEventManager2>("GAMEEVENTSMANAGER", sharedobj::engine(), 2);
    g_IGameEventManager = BruteforceInterface<IGameEventManager>("GAMEEVENTSMANAGER", sharedobj::engine(), 1);
    g_IBaseClient       = BruteforceInterface<IBaseClientDLL>("VClient", sharedobj::client());
    g_ITrace            = BruteforceInterface<IEngineTrace>("EngineTraceClient", sharedobj::engine());
    g_IInputSystem      = BruteforceInterface<IInputSystem>("InputSystemVersion", sharedobj::inputsystem());
    g_IServerTools      = BruteforceInterface<CServerTools>("VSERVERTOOLS", sharedobj::server());

    logging::Info("Initing SteamAPI");
    GetHSteamPipe_t GetHSteamPipe = reinterpret_cast<GetHSteamPipe_t>(dlsym(sharedobj::steamapi().lmap, "SteamAPI_GetHSteamPipe"));
    HSteamPipe sp                 = GetHSteamPipe();
    if (!sp)
    {
        logging::Info("Connecting to Steam User");
        sp = g_ISteamClient->CreateSteamPipe();
    }
    GetHSteamUser_t GetHSteamUser = reinterpret_cast<GetHSteamUser_t>(dlsym(sharedobj::steamapi().lmap, "SteamAPI_GetHSteamUser"));
    HSteamUser su                 = GetHSteamUser();
    if (!su)
    {
        logging::Info("Connecting to Steam User");
        su = g_ISteamClient->ConnectToGlobalUser(sp);
    }
    logging::Info("Inited SteamAPI");

    g_IVModelRender = BruteforceInterface<IVModelRender>("VEngineModel", sharedobj::engine(), 16);
    g_ISteamFriends = nullptr;
    g_IEngineVGui   = BruteforceInterface<IEngineVGui>("VEngineVGui", sharedobj::engine());
    IF_GAME(IsTF2())
    {
        uintptr_t sig_steamapi = gSignatures.GetEngineSignature("55 0F 57 C0 89 E5 83 EC 18 F3 0F 11 05 ? ? ? ? F3 0F 11 05 ? ? ? ? F3 0F 10 05 ? ? ? ? C7 04 24 ? ? ? ? F3 0F 11 05 ? ? ? ? F3 0F 11 05 ? ? ? ? E8 ? ? ? ? C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? C7 04 24 ? ? ? ? E8 ? ? ? ? C9 C3 90 90 90 90 90 55 0F 57 C0 89 E5 83 EC 28");
        logging::Info("SteamAPI: 0x%08x", sig_steamapi);
        void **SteamAPI_engine = *reinterpret_cast<void ***>(sig_steamapi + 36);
        g_ISteamFriends        = reinterpret_cast<ISteamFriends *>(SteamAPI_engine[2]);
    }
    if (g_ISteamFriends == nullptr)
    {
        // FIXME SIGNATURE
        g_ISteamFriends = g_ISteamClient->GetISteamFriends(su, sp, "SteamFriends002");
    }
    rg_GlobalVars   = *reinterpret_cast<CGlobalVarsBase ***>(gSignatures.GetClientSignature("8B 45 08 8B 15 ? ? ? ? F3 0F 10 88 ? ? ? ?") + 5);
    g_IPrediction   = BruteforceInterface<IPrediction>("VClientPrediction", sharedobj::client());
    g_IGameMovement = BruteforceInterface<IGameMovement>("GameMovement", sharedobj::client());
    IF_GAME(IsTF2())
    {
        g_IInput = **(reinterpret_cast<IInput ***>((uintptr_t) 1 + gSignatures.GetClientSignature("A1 ? ? ? ? C6 05 ? ? ? ? ? 8B 10 89 04 24 FF 92 ? ? ? ? A1")));
    }
    g_ISteamUser       = g_ISteamClient->GetISteamUser(su, sp, "SteamUser018");
    g_IModelInfo       = BruteforceInterface<IVModelInfoClient>("VModelInfoClient", sharedobj::engine());
    g_IBaseClientState = *(reinterpret_cast<CBaseClientState **>(gSignatures.GetEngineSignature("C7 04 24 ? ? ? ? E8 ? ? ? ? C7 04 24 ? ? ? ? 89 44 24 04 E8 ? ? ? ? A1 ? ? ? ?") + 3));
    logging::Info("BaseClientState: 0x%08x", g_IBaseClientState);
    g_IAchievementMgr = g_IEngine->GetAchievementMgr();
    g_ISteamUserStats = g_ISteamClient->GetISteamUserStats(su, sp, "STEAMUSERSTATS_INTERFACE_VERSION011");
    IF_GAME(IsTF2())
    {
        uintptr_t sig          = gSignatures.GetClientSignature("A3 ? ? ? ? C3 8D 74 26 00 B8 FF FF FF FF 5D A3 ? ? ? ? C3");
        g_PredictionRandomSeed = *reinterpret_cast<int **>(sig + (uintptr_t) 1);

        uintptr_t g_pGameRules_sig = gSignatures.GetClientSignature("C7 03 ? ? ? ? 89 1D ? ? ? ? 83 C4 14 5B 5D C3");
        rg_pGameRules              = *reinterpret_cast<CGameRules ***>(g_pGameRules_sig + 8);
    }
    g_IMaterialSystem = BruteforceInterface<IMaterialSystemFixed>("VMaterialSystem", sharedobj::materialsystem());
    g_IMDLCache       = BruteforceInterface<IMDLCache>("MDLCache", sharedobj::datacache());

    g_IPanel         = BruteforceInterface<vgui::IPanel>("VGUI_Panel", sharedobj::vgui2());
    g_ILocalize      = BruteforceInterface<vgui::ILocalize>("VGUI_Localize", sharedobj::vgui2());
    g_pUniformStream = **(IUniformRandomStream ***) (gSignatures.GetVstdSignature("A3 ? ? ? ? C3 89 F6") + 0x1);
    g_IToolFramework = BruteforceInterface<IToolFrameworkInternal>("VTOOLFRAMEWORKVERSION", sharedobj::engine());
#if ENABLE_VISUALS
    g_IVDebugOverlay    = BruteforceInterface<IVDebugOverlay>("VDebugOverlay", sharedobj::engine());
    g_ISurface          = BruteforceInterface<vgui::ISurface>("VGUI_Surface", sharedobj::vguimatsurface());
    g_IStudioRender     = BruteforceInterface<IStudioRender>("VStudioRender", sharedobj::studiorender());
    g_IVRenderView      = BruteforceInterface<IVRenderView>("VEngineRenderView", sharedobj::engine());
    g_IMaterialSystemHL = (IMaterialSystem *) g_IMaterialSystem;
    IF_GAME(IsTF2())
    {
        g_pScreenSpaceEffects           = **(IScreenSpaceEffectManager ***) (gSignatures.GetClientSignature("8D 74 26 00 55 89 E5 57 56 53 83 EC 1C 8B 5D 08 8B 7D 0C 8B 75 10 ") + 0x1c3);
        g_ppScreenSpaceRegistrationHead = *(CScreenSpaceEffectRegistration ***) (gSignatures.GetClientSignature("8B 1D ? ? ? ? 85 DB 74 25 ") + 2);
    }
#endif
    logging::Info("Finding HUD");
    IF_GAME(IsCSS())
    {
        logging::Info("FATAL: Signatures not defined for CSS - HUD");
        g_CHUD = nullptr;
    }
    else
    {
        uintptr_t hud_sig = gSignatures.GetClientSignature("C7 04 24 ? ? ? ? D9 9D ? ? ? ? E8 ? ? ? ? 85 C0 74 3B") + 3;
        g_CHUD            = *reinterpret_cast<CHud **>(hud_sig);
        logging::Info("HUD 0x%08x 0x%08x", hud_sig, g_CHUD);
    }
}
