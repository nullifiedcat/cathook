/*
 * entity.h
 *
 *  Created on: Oct 6, 2016
 *      Author: nullifiedcat
 */

#pragma once

#include <core/logging.hpp>

class IClientEntity;

#define NET_VAR(entity, offset, type) (*(reinterpret_cast<type *>(reinterpret_cast<uint64_t>(entity) + (offset))))

#define NET_INT(entity, offset) NET_VAR(entity, offset, int)

#define NET_FLOAT(entity, offset) NET_VAR(entity, offset, float)

#define NET_BYTE(entity, offset) NET_VAR(entity, offset, unsigned char)

#define NET_VECTOR(entity, offset) NET_VAR(entity, offset, Vector)

typedef unsigned int offset_t;

/*template<typename T>
inline T GetVar(IClientEntity* ent, unsigned int offset) {
    int nullv = 0;
    if (ent == 0) return *(reinterpret_cast<T*>(&nullv));
    //logging::Info("GetEntityValue 0x%08x, 0x%08x", ent, offset);
    return *(reinterpret_cast<T*>((unsigned int)ent + offset));
}

template<typename T>
void SetVar(IClientEntity* ent, unsigned int offset, T value) {
    *(reinterpret_cast<T*>((unsigned int)ent + offset)) = value;
}*/

void InitNetVars();

class NetVars
{
public:
    void Init();
    offset_t iTeamNum;
    offset_t iFlags;
    offset_t iHealth;

    offset_t m_vecOrigin;

    // sentry
    offset_t m_iAmmoShells;       // sentry shells
    offset_t m_iAmmoRockets;      // use only with if (GetLevel() == 3)
    offset_t m_iSentryState;      // sentry state
    offset_t m_bPlayerControlled; // controlled via wrangler and sort
    offset_t m_bDisabled;         // this should be sentry only, not sure

    // dispenser
    offset_t m_iAmmoMetal; // dispenser metal reserve

    // round timer
    offset_t m_nSetupTimeLength;
    offset_t m_nState;

    offset_t iLifeState;
    offset_t iCond;
    offset_t iCond1;
    offset_t iCond2;
    offset_t iCond3;
    offset_t iClass;
    offset_t vViewOffset;
    offset_t hActiveWeapon;
    offset_t flChargedDamage;

    // any building
    offset_t m_iUpgradeMetal;           // upgrade metal on any building
    offset_t m_flPercentageConstructed; // use only with        if (IsBuilding())
    offset_t iUpgradeLevel;             // any building
    offset_t m_iUpgradeMetalRequired;
    offset_t m_hBuilder;
    offset_t m_bCanPlace;
    offset_t m_iObjectType;
    offset_t m_bMiniBuilding;
    offset_t m_bHasSapper;
    offset_t m_bPlacing;
    offset_t m_bBuilding;
    offset_t m_bPlasmaDisable;
    offset_t m_bCarryDeploy;

    // teleporter
    offset_t m_iTeleState; // teleport state [1 = idle, 2 = active, 3 = teleporting, 4 = charging]
    offset_t m_flTeleRechargeTime;
    offset_t m_flTeleCurrentRechargeDuration;
    offset_t m_iTeleTimesUsed;
    offset_t m_flTeleYawToExit;
    offset_t m_bMatchBuilding;

    offset_t iPipeType;
    offset_t iBuildingHealth;
    offset_t iBuildingMaxHealth;
    offset_t m_iAmmo;
    offset_t m_iPrimaryAmmoType;
    offset_t m_iSecondaryAmmoType;
    offset_t iHitboxSet;
    offset_t vVelocity;
    offset_t bGlowEnabled;
    offset_t iGlowIndex;
    offset_t iReloadMode;
    offset_t res_iMaxHealth;
    offset_t flNextAttack;
    offset_t iNextMeleeCrit;
    offset_t flNextPrimaryAttack;
    offset_t flNextSecondaryAttack;
    offset_t iNextThinkTick;
    offset_t m_iClip1;
    offset_t m_iClip2;
    // offset_t flReloadPriorNextFire;
    // offset_t flObservedCritChance;
    offset_t nTickBase;
    // offset_t iDecapitations;
    offset_t res_iMaxBuffedHealth;
    offset_t bRespawning;
    offset_t iItemDefinitionIndex;
    offset_t AttributeList;

    offset_t vecPunchAngle;
    offset_t vecPunchAngleVel;

    offset_t iObserverMode;
    offset_t hObserverTarget;

    offset_t flChargeBeginTime;
    offset_t flLastFireTime;
    offset_t flObservedCritChance;
    offset_t hThrower;
    offset_t hMyWeapons;

    offset_t Rocket_iDeflected;
    offset_t Grenade_iDeflected;
    offset_t Rocket_bCritical;
    offset_t Grenade_bCritical;
    offset_t m_DmgRadius;

    offset_t bDistributed;

    offset_t angEyeAngles;
    offset_t deadflag;
    offset_t nForceTauntCam;

    offset_t iDefaultFOV;
    offset_t iFOV;
    offset_t _condition_bits;
    offset_t res_iPlayerClass;

    offset_t hOwner;
    offset_t iWeaponState;
    offset_t iCritMult; // TF2C

    offset_t flChargeLevel;
    offset_t bChargeRelease;

    offset_t m_flStealthNoAttackExpire;
    offset_t m_iCrits;
    offset_t m_flDuckTimer;
    offset_t m_bDucked;
    offset_t m_angEyeAngles;
    offset_t m_bReadyToBackstab;
    offset_t m_Collision;
    offset_t res_iTeam;
    offset_t res_iScore;
    offset_t res_bAlive;
    offset_t m_nChargeResistType;
    offset_t m_hHealingTarget;
    offset_t m_flChargeLevel; // Medigun
    offset_t m_flChargeMeter; // Demoman Shield

    offset_t m_rgflCoordinateFrame;
    offset_t m_bFeignDeathReady;
    offset_t m_bCarryingObject;
    offset_t m_hCarriedObject;

    offset_t m_iTauntConcept;
    offset_t m_iTauntIndex;
    offset_t m_bViewingCYOAPDA;
    offset_t m_angEyeAnglesLocal;
    offset_t m_nSequence;
    offset_t m_flPoseParameter;
    offset_t m_flModelScale;
    offset_t m_flEncodedController;
    offset_t m_flSimulationTime;
    offset_t m_flAnimTime;
    offset_t m_flCycle;
    offset_t m_angRotation;

    offset_t m_hOwnerEntity;

    offset_t m_nStreaks_Player;
    offset_t m_nStreaks_Resource;
    offset_t m_iPing_Resource;
    offset_t m_iKills_Resource;
    offset_t m_iDeaths_Resource;
    offset_t m_iHealth_Resource;
    offset_t m_iTotalScore_Resource;
    offset_t m_iMaxHealth_Resource;
    offset_t m_iMaxBuffedHealth_Resource;
    offset_t m_iPlayerClass_Resource;
    offset_t m_iActiveDominations_Resource;
    offset_t m_flNextRespawnTime_Resource;
    offset_t m_iDamage_Resource;
    offset_t m_iDamageAssist_Resource;
    offset_t m_iHealing_Resource;
    offset_t m_iHealingAssist_Resource;
    offset_t m_iPlayerLevel_Resource;

    offset_t m_nFlagType;
    offset_t m_nFlagStatus;

    offset_t m_bTeamCanCap;
    offset_t m_iNumControlPoints;
    offset_t m_vCPPositions;
    offset_t m_iOwningTeam;
    offset_t m_bCPLocked;
    offset_t m_bPlayingMiniRounds;
    offset_t m_bInMiniRound;
    offset_t m_iPreviousPoints;
    offset_t m_iBaseControlPoints;

    offset_t m_iPlayerIndex;
    offset_t m_hTargetPlayer;
    offset_t m_flResetTime;
    offset_t m_flMaxspeed;
};

extern NetVars netvar;
