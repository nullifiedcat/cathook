/*
a * entity.cpp
 *
 *  Created on: Oct 6, 2016
 *      Author: nullifiedcat
 */

#include "copypasted/Netvar.h"
#include "common.hpp"

NetVars netvar;

void NetVars::Init()
{
    this->iFlags               = gNetvars.get_offset("DT_BasePlayer", "m_fFlags");
    this->iHealth              = gNetvars.get_offset("DT_BasePlayer", "m_iHealth");
    this->iLifeState           = gNetvars.get_offset("DT_BasePlayer", "m_lifeState");
    this->iTeamNum             = gNetvars.get_offset("DT_BaseEntity", "m_iTeamNum");
    this->vViewOffset          = gNetvars.get_offset("DT_BasePlayer", "localdata", "m_vecViewOffset[0]");
    this->hActiveWeapon        = gNetvars.get_offset("DT_BaseCombatCharacter", "m_hActiveWeapon");
    this->hMyWeapons           = gNetvars.get_offset("DT_BaseCombatCharacter", "m_hMyWeapons");
    this->iHitboxSet           = gNetvars.get_offset("DT_BaseAnimating", "m_nHitboxSet");
    this->vVelocity            = gNetvars.get_offset("DT_BasePlayer", "localdata", "m_vecVelocity[0]");
    this->m_iAmmo              = gNetvars.get_offset("DT_BasePlayer", "localdata", "m_iAmmo");
    this->m_iPrimaryAmmoType   = gNetvars.get_offset("DT_BaseCombatWeapon", "LocalWeaponData", "m_iPrimaryAmmoType");
    this->m_iSecondaryAmmoType = gNetvars.get_offset("DT_BaseCombatWeapon", "LocalWeaponData", "m_iSecondaryAmmoType");
    this->m_iClip1             = gNetvars.get_offset("DT_BaseCombatWeapon", "LocalWeaponData", "m_iClip1");
    this->m_iClip2             = gNetvars.get_offset("DT_BaseCombatWeapon", "LocalWeaponData", "m_iClip2");
    this->m_Collision          = gNetvars.get_offset("DT_BaseEntity", "m_Collision");
    this->m_flSimulationTime   = gNetvars.get_offset("DT_BaseEntity", "m_flSimulationTime");
    this->m_flAnimTime         = gNetvars.get_offset("DT_BaseEntity", "AnimTimeMustBeFirst", "m_flAnimTime");
    this->m_flCycle            = gNetvars.get_offset("DT_BaseAnimating", "serveranimdata", "m_flCycle");
    this->m_angRotation        = gNetvars.get_offset("DT_BaseEntity", "m_angRotation");
    this->m_vecOrigin          = gNetvars.get_offset("DT_BaseEntity", "m_vecOrigin");

    IF_GAME(IsTF2())
    {
        this->m_flMaxspeed         = gNetvars.get_offset("DT_BasePlayer", "m_flMaxspeed");
        res_iTeam                  = gNetvars.get_offset("DT_TFPlayerResource", "baseclass", "m_iTeam");
        res_bAlive                 = gNetvars.get_offset("DT_TFPlayerResource", "baseclass", "m_bAlive");
        this->res_iMaxBuffedHealth = gNetvars.get_offset("DT_TFPlayerResource", "m_iMaxBuffedHealth");
        m_angEyeAngles             = gNetvars.get_offset("DT_TFPlayer", "tfnonlocaldata", "m_angEyeAngles[0]");
        m_angEyeAnglesLocal        = gNetvars.get_offset("DT_TFPlayer", "tflocaldata", "m_angEyeAngles[0]");
        this->bGlowEnabled         = gNetvars.get_offset("DT_TFPlayer", "m_bGlowEnabled");
        // this->iDecapitations = gNetvars.get_offset("DT_TFPlayer", "m_Shared",
        // "m_iDecapitations");  this->flReloadPriorNextFire =
        // gNetvars.get_offset("DT_TFWeaponBase", "LocalActiveTFWeaponData",
        // "m_flReloadPriorNextFire");  this->flObservedCritChance =
        // gNetvars.get_offset("DT_TFWeaponBase", "LocalActiveTFWeaponData",
        // "m_flObservedCritChance");
        this->iItemDefinitionIndex      = gNetvars.get_offset("DT_EconEntity", "m_AttributeManager", "m_Item", "m_iItemDefinitionIndex");
        this->AttributeList             = gNetvars.get_offset("DT_EconEntity", "m_AttributeManager", "m_Item",
                                                  "m_AttributeList"); // hmmm
        this->flChargeBeginTime         = gNetvars.get_offset("DT_WeaponPipebombLauncher", "PipebombLauncherLocalData", "m_flChargeBeginTime");
        this->flLastFireTime            = gNetvars.get_offset("DT_TFWeaponBase", "LocalActiveTFWeaponData", "m_flLastFireTime");
        this->flObservedCritChance      = gNetvars.get_offset("DT_TFWeaponBase", "LocalActiveTFWeaponData", "m_flObservedCritChance");
        this->bDistributed              = gNetvars.get_offset("DT_CurrencyPack", "m_bDistributed");
        this->_condition_bits           = gNetvars.get_offset("DT_TFPlayer", "m_Shared", "m_ConditionList", "_condition_bits");
        this->m_flStealthNoAttackExpire = gNetvars.get_offset("DT_TFPlayer", "m_Shared", "tfsharedlocaldata", "m_flStealthNoAttackExpire");
        this->m_iCrits                  = gNetvars.get_offset("DT_TFPlayer", "m_Shared", "tfsharedlocaldata", "m_RoundScoreData", "m_iCrits");
        this->m_nChargeResistType       = gNetvars.get_offset("DT_WeaponMedigun", "m_nChargeResistType");
        this->m_hHealingTarget          = gNetvars.get_offset("DT_WeaponMedigun", "m_hHealingTarget");
        this->m_flChargeLevel           = gNetvars.get_offset("DT_WeaponMedigun", "NonLocalTFWeaponMedigunData", "m_flChargeLevel");
        this->m_flChargeMeter           = gNetvars.get_offset("DT_TFPlayer", "m_Shared", "m_flChargeMeter");
        this->m_bFeignDeathReady        = gNetvars.get_offset("DT_TFPlayer", "m_Shared", "m_bFeignDeathReady");
        this->m_bCarryingObject         = gNetvars.get_offset("DT_TFPlayer", "m_Shared", "m_bCarryingObject");
        this->m_hCarriedObject          = gNetvars.get_offset("DT_TFPlayer", "m_Shared", "m_hCarriedObject");
        this->m_nSequence               = gNetvars.get_offset("DT_BaseAnimating", "m_nSequence");
        this->m_flPoseParameter         = gNetvars.get_offset("DT_BaseAnimating", "m_flPoseParameter");
        this->m_flModelScale            = gNetvars.get_offset("DT_BaseAnimating", "m_flModelScale");
        this->m_flEncodedController     = gNetvars.get_offset("DT_BaseAnimating", "m_flEncodedController");
        this->m_iTauntIndex             = gNetvars.get_offset("DT_TFPlayer", "m_Shared", "m_iTauntIndex");
        this->m_iTauntConcept           = gNetvars.get_offset("DT_TFPlayer", "m_Shared", "m_iTauntConcept");
        this->m_bViewingCYOAPDA         = gNetvars.get_offset("DT_TFPlayer", "m_bViewingCYOAPDA");
    }
    this->res_iScore = gNetvars.get_offset("DT_TFPlayerResource", "baseclass", "m_iScore");
    IF_GAME(IsTF())
    {
        this->m_hOwnerEntity     = gNetvars.get_offset("DT_BaseEntity", "m_hOwnerEntity");
        this->res_iMaxHealth     = gNetvars.get_offset("DT_TFPlayerResource", "m_iMaxHealth");
        this->res_iPlayerClass   = gNetvars.get_offset("DT_TFPlayerResource", "m_iPlayerClass");
        this->m_bReadyToBackstab = gNetvars.get_offset("DT_TFWeaponKnife", "m_bReadyToBackstab");
        this->m_bDucked          = gNetvars.get_offset("DT_TFPlayer", "localdata", "m_Local", "m_bDucked");
        this->m_flDuckTimer      = gNetvars.get_offset("DT_TFPlayer", "m_Shared", "m_flDuckTimer");
        this->iCond              = gNetvars.get_offset("DT_TFPlayer", "m_Shared", "m_nPlayerCond");
        this->iCond1             = gNetvars.get_offset("DT_TFPlayer", "m_Shared", "m_nPlayerCondEx");
        this->iCond2             = gNetvars.get_offset("DT_TFPlayer", "m_Shared", "m_nPlayerCondEx2");
        this->iCond3             = gNetvars.get_offset("DT_TFPlayer", "m_Shared", "m_nPlayerCondEx3");
        this->iClass             = gNetvars.get_offset("DT_TFPlayer", "m_PlayerClass", "m_iClass");
        this->flChargedDamage    = gNetvars.get_offset("DT_TFSniperRifle", "SniperRifleLocalData", "m_flChargedDamage");

        // sentry
        this->m_iAmmoShells       = gNetvars.get_offset("DT_ObjectSentrygun", "m_iAmmoShells");
        this->m_iAmmoRockets      = gNetvars.get_offset("DT_ObjectSentrygun", "m_iAmmoRockets");
        this->m_iSentryState      = gNetvars.get_offset("DT_ObjectSentrygun", "m_iState");
        this->m_bPlayerControlled = gNetvars.get_offset("DT_ObjectSentrygun", "m_bPlayerControlled");
        this->m_bDisabled         = gNetvars.get_offset("DT_BaseObject", "m_bDisabled");

        // dispenser
        this->m_iAmmoMetal = gNetvars.get_offset("DT_ObjectDispenser", "m_iAmmoMetal");

        // Round timer
        this->m_nSetupTimeLength = gNetvars.get_offset("DT_TeamRoundTimer", "m_nSetupTimeLength");
        this->m_nState           = gNetvars.get_offset("DT_TeamRoundTimer", "m_nState");

        // any building
        this->m_iUpgradeMetal           = gNetvars.get_offset("DT_BaseObject", "m_iUpgradeMetal");
        this->m_flPercentageConstructed = gNetvars.get_offset("DT_BaseObject", "m_flPercentageConstructed");
        this->iUpgradeLevel             = gNetvars.get_offset("DT_BaseObject", "m_iUpgradeLevel");
        this->m_iUpgradeMetalRequired   = gNetvars.get_offset("DT_BaseObject", "m_iUpgradeMetalRequired");
        this->m_hBuilder                = gNetvars.get_offset("DT_BaseObject", "m_hBuilder");
        this->m_bCanPlace               = gNetvars.get_offset("DT_BaseObject", "m_bServerOverridePlacement");
        this->m_bBuilding               = gNetvars.get_offset("DT_BaseObject", "m_bBuilding");
        this->m_iObjectType             = gNetvars.get_offset("DT_BaseObject", "m_iObjectType");
        this->m_bHasSapper              = gNetvars.get_offset("DT_BaseObject", "m_bHasSapper");
        this->m_bPlacing                = gNetvars.get_offset("DT_BaseObject", "m_bPlacing");
        this->m_bMiniBuilding           = gNetvars.get_offset("DT_BaseObject", "m_bMiniBuilding");
        this->m_bPlasmaDisable          = gNetvars.get_offset("DT_BaseObject", "m_bPlasmaDisable");

        // any building
        this->iUpgradeLevel   = gNetvars.get_offset("DT_BaseObject", "m_iUpgradeLevel");
        this->m_hBuilder      = gNetvars.get_offset("DT_BaseObject", "m_hBuilder");
        this->m_bCanPlace     = gNetvars.get_offset("DT_BaseObject", "m_bServerOverridePlacement");
        this->m_bBuilding     = gNetvars.get_offset("DT_BaseObject", "m_bBuilding");
        this->m_bCarryDeploy  = gNetvars.get_offset("DT_BaseObject", "m_bCarryDeploy");
        this->m_iObjectType   = gNetvars.get_offset("DT_BaseObject", "m_iObjectType");
        this->m_bHasSapper    = gNetvars.get_offset("DT_BaseObject", "m_bHasSapper");
        this->m_bPlacing      = gNetvars.get_offset("DT_BaseObject", "m_bPlacing");
        this->m_bMiniBuilding = gNetvars.get_offset("DT_BaseObject", "m_bMiniBuilding");

        // teleporter
        this->m_iTeleState                    = gNetvars.get_offset("DT_ObjectTeleporter", "m_iState");
        this->m_flTeleRechargeTime            = gNetvars.get_offset("DT_ObjectTeleporter", "m_flRechargeTime");
        this->m_flTeleCurrentRechargeDuration = gNetvars.get_offset("DT_ObjectTeleporter", "m_flCurrentRechargeDuration");
        this->m_iTeleTimesUsed                = gNetvars.get_offset("DT_ObjectTeleporter", "m_iTimesUsed");
        this->m_flTeleYawToExit               = gNetvars.get_offset("DT_ObjectTeleporter", "m_flYawToExit");
        this->m_bMatchBuilding                = gNetvars.get_offset("DT_ObjectTeleporter", "m_bMatchBuilding");

        // CTF Flag
        this->m_nFlagType   = gNetvars.get_offset("DT_CaptureFlag", "m_nType");
        this->m_nFlagStatus = gNetvars.get_offset("DT_CaptureFlag", "m_nFlagStatus");

        // ObjectiveResource
        this->m_bTeamCanCap        = gNetvars.get_offset("DT_BaseTeamObjectiveResource", "m_bTeamCanCap");
        this->m_iNumControlPoints  = gNetvars.get_offset("DT_BaseTeamObjectiveResource", "m_iNumControlPoints");
        this->m_vCPPositions       = gNetvars.get_offset("DT_BaseTeamObjectiveResource", "m_vCPPositions[0]");
        this->m_iOwningTeam        = gNetvars.get_offset("DT_BaseTeamObjectiveResource", "m_iOwner");
        this->m_bCPLocked          = gNetvars.get_offset("DT_BaseTeamObjectiveResource", "m_bCPLocked");
        this->m_bPlayingMiniRounds = gNetvars.get_offset("DT_BaseTeamObjectiveResource", "m_bPlayingMiniRounds");
        this->m_bInMiniRound       = gNetvars.get_offset("DT_BaseTeamObjectiveResource", "m_bInMiniRound");
        this->m_iPreviousPoints    = gNetvars.get_offset("DT_BaseTeamObjectiveResource", "m_iPreviousPoints");
        this->m_iBaseControlPoints = gNetvars.get_offset("DT_BaseTeamObjectiveResource", "m_iBaseControlPoints");

        this->m_DmgRadius                   = gNetvars.get_offset("DT_BaseGrenade", "m_DmgRadius");
        this->iPipeType                     = gNetvars.get_offset("DT_TFProjectile_Pipebomb", "m_iType");
        this->iBuildingHealth               = gNetvars.get_offset("DT_BaseObject", "m_iHealth");
        this->iBuildingMaxHealth            = gNetvars.get_offset("DT_BaseObject", "m_iMaxHealth");
        this->iReloadMode                   = gNetvars.get_offset("DT_TFWeaponBase", "m_iReloadMode");
        this->Rocket_iDeflected             = gNetvars.get_offset("DT_TFBaseRocket", "m_iDeflected");
        this->Grenade_iDeflected            = gNetvars.get_offset("DT_TFWeaponBaseGrenadeProj", "m_iDeflected");
        this->nForceTauntCam                = gNetvars.get_offset("DT_TFPlayer", "m_nForceTauntCam");
        this->Rocket_bCritical              = gNetvars.get_offset("DT_TFProjectile_Rocket", "m_bCritical");
        this->Grenade_bCritical             = gNetvars.get_offset("DT_TFWeaponBaseGrenadeProj", "m_bCritical");
        this->angEyeAngles                  = gNetvars.get_offset("DT_TFPlayer", "tfnonlocaldata", "m_angEyeAngles[0]");
        this->iWeaponState                  = gNetvars.get_offset("DT_WeaponMinigun", "m_iWeaponState");
        this->flChargeLevel                 = gNetvars.get_offset("DT_WeaponMedigun", "NonLocalTFWeaponMedigunData", "m_flChargeLevel");
        this->bChargeRelease                = gNetvars.get_offset("DT_WeaponMedigun", "m_bChargeRelease");
        this->m_nStreaks_Player             = gNetvars.get_offset("DT_TFPlayer", "m_Shared", "m_nStreaks");
        this->m_nStreaks_Resource           = gNetvars.get_offset("DT_TFPlayerResource", "m_iStreaks");
        this->m_iKills_Resource             = gNetvars.get_offset("DT_TFPlayerResource", "baseclass", "m_iScore");
        this->m_iPing_Resource              = gNetvars.get_offset("DT_TFPlayerResource", "baseclass", "m_iPing");
        this->m_iDeaths_Resource            = gNetvars.get_offset("DT_TFPlayerResource", "baseclass", "m_iDeaths");
        this->m_iHealth_Resource            = gNetvars.get_offset("DT_TFPlayerResource", "baseclass", "m_iHealth");
        this->m_iTotalScore_Resource        = gNetvars.get_offset("DT_TFPlayerResource", "m_iTotalScore");
        this->m_iMaxHealth_Resource         = gNetvars.get_offset("DT_TFPlayerResource", "m_iMaxHealth");
        this->m_iMaxBuffedHealth_Resource   = gNetvars.get_offset("DT_TFPlayerResource", "m_iMaxBuffedHealth");
        this->m_iPlayerClass_Resource       = gNetvars.get_offset("DT_TFPlayerResource", "m_iPlayerClass");
        this->m_iActiveDominations_Resource = gNetvars.get_offset("DT_TFPlayerResource", "m_iActiveDominations");
        this->m_flNextRespawnTime_Resource  = gNetvars.get_offset("DT_TFPlayerResource", "m_flNextRespawnTime");
        this->m_iDamage_Resource            = gNetvars.get_offset("DT_TFPlayerResource", "m_iDamage");
        this->m_iDamageAssist_Resource      = gNetvars.get_offset("DT_TFPlayerResource", "m_iDamageAssist");
        this->m_iHealing_Resource           = gNetvars.get_offset("DT_TFPlayerResource", "m_iHealing");
        this->m_iHealingAssist_Resource     = gNetvars.get_offset("DT_TFPlayerResource", "m_iHealingAssist");
        this->m_iPlayerLevel_Resource       = gNetvars.get_offset("DT_TFPlayerResource", "m_iPlayerLevel");
        this->m_iPlayerIndex                = gNetvars.get_offset("DT_TFRagdoll", "m_iPlayerIndex");

        // Gargoyle
        this->m_hTargetPlayer = gNetvars.get_offset("DT_CHalloweenGiftPickup", "m_hTargetPlayer");

        // Flag
        this->m_flResetTime = gNetvars.get_offset("DT_CaptureFlag", "m_flResetTime");
    }
    IF_GAME(IsTF2C())
    {
        this->iCritMult   = gNetvars.get_offset("DT_TFPlayer", "m_Shared", "m_iCritMult");
        this->bRespawning = gNetvars.get_offset("DT_WeaponSpawner", "m_bRespawning");
    }
    this->flNextAttack          = gNetvars.get_offset("DT_BaseCombatCharacter", "bcc_localdata", "m_flNextAttack");
    this->flNextPrimaryAttack   = gNetvars.get_offset("DT_BaseCombatWeapon", "LocalActiveWeaponData", "m_flNextPrimaryAttack");
    this->flNextSecondaryAttack = gNetvars.get_offset("DT_BaseCombatWeapon", "LocalActiveWeaponData", "m_flNextSecondaryAttack");
    this->iNextThinkTick        = gNetvars.get_offset("DT_BaseCombatWeapon", "LocalActiveWeaponData", "m_nNextThinkTick");
    this->nTickBase             = gNetvars.get_offset("DT_BasePlayer", "localdata", "m_nTickBase");
    this->vecPunchAngle         = gNetvars.get_offset("DT_BasePlayer", "localdata", "m_Local", "m_vecPunchAngle");
    this->vecPunchAngleVel      = gNetvars.get_offset("DT_BasePlayer", "localdata", "m_Local", "m_vecPunchAngleVel");
    this->hThrower              = gNetvars.get_offset("DT_BaseGrenade", "m_hThrower");
    this->iObserverMode         = gNetvars.get_offset("DT_BasePlayer", "m_iObserverMode");
    this->hObserverTarget       = gNetvars.get_offset("DT_BasePlayer", "m_hObserverTarget");
    this->deadflag              = gNetvars.get_offset("DT_BasePlayer", "pl", "deadflag");
    this->iFOV                  = gNetvars.get_offset("DT_BasePlayer", "m_iFOV");
    this->iDefaultFOV           = gNetvars.get_offset("DT_BasePlayer", "m_iDefaultFOV");
    this->hOwner                = gNetvars.get_offset("DT_BaseCombatWeapon", "m_hOwner");
    this->m_rgflCoordinateFrame = 0x324;
}

void InitNetVars()
{
    netvar.Init();
}
