#include "common.hpp"
#include "bone_setup.h"
#include "animationlayer.h"
#include <boost/algorithm/string.hpp>

namespace setupbones_reconst
{
#define MAX_OVERLAYS 15

static settings::Boolean remove_taunts("remove.taunts", "false");

// This is to fix hitboxes while taunting. In most cases. Sometimes these sequences suddenly
// Have normal hitboxes.

// clang-format off
static std::vector<int> ignore_sequences{
    125, 147, 127, 150, 118, 165, 131, 154, 124, 173, 251, 279, 249,
    277, 252, 280, 256, 282, 257, 285, 273, 289, 299, 301, 247, 275,
    303, 347, 308, 354, 298, 334, 313, 359, 311, 357, 338, 383, 343,
    388, 347, 392, 353, 424, 355, 426, 303, 304, 349, 342, 381, 394,
    396, 404, 408, 416, 295, 225, 193, 205, 139, 185, 129, 152, 127,
    309, 223, 171, 198, 209, 138, 183, 184, 137, 207, 196, 307, 150,
    118, 165, 286, 210, 151, 176, 194, 124, 187, 154, 288, 212, 153,
    178, 126, 189, 173, 220, 167, 192, 202, 136, 180, 251, 279, 305,
    243, 204, 182, 222, 249, 277, 221, 168, 241, 203, 181, 257, 285,
    330, 324, 236, 188, 245, 273, 289, 269, 346, 267, 271, 253, 259,
    265, 250, 206, 275, 291, 334, 263, 244, 200, 348, 297, 299, 356,
    256, 344, 208, 301, 358, 258, 247, 239, 228, 314, 322, 237, 389,
    377, 238, 287, 308, 354, 292, 382, 306, 327, 384, 294, 282, 372,
    296, 233, 317, 386, 284, 374, 235, 313, 399, 401, 248, 359, 397,
    385, 246, 332, 428, 412, 360, 343, 388, 435, 337, 329, 417, 274,
    365, 345, 347, 392, 441, 341, 333, 421, 349, 278, 373, 353, 424,
    450, 348, 438, 355, 286, 387, 426, 350, 452, 440, 288, 357, 389,
    281, 334, 336, 351, 430, 439, 448, 342, 344, 130, 212, 280, 282,
    322, 335, 340, 269, 356, 369, 375, 377, 383, 228, 331, 347, 351,
    299, 408, 423, 430, 434
};
//clang-format on

void GetSkeleton(IClientEntity *ent, CStudioHdr *pStudioHdr, Vector pos[], Quaternion q[], int boneMask)
{
    if (!pStudioHdr)
        return;

    if (!pStudioHdr->SequencesAvailable())
    {
        return;
    }

    static uintptr_t m_pIK_offset = 0x568;
    CIKContext **m_pIk            = (reinterpret_cast<CIKContext **>(reinterpret_cast<uint64_t>(ent) + (m_pIK_offset)));

    static uintptr_t m_AnimOverlay_offset       = 0x894;
    CUtlVector<C_AnimationLayer> &m_AnimOverlay = NET_VAR(ent, m_AnimOverlay_offset, CUtlVector<C_AnimationLayer>);

    IBoneSetup boneSetup(pStudioHdr, boneMask, &NET_FLOAT(ent, netvar.m_flPoseParameter));
    boneSetup.InitPose(pos, q);

    boneSetup.AccumulatePose(pos, q, NET_INT(ent, netvar.m_nSequence), NET_FLOAT(ent, netvar.m_flCycle), 1.0, g_GlobalVars->curtime, *m_pIk);

    // sort the layers
    int layer[MAX_OVERLAYS] = {};
    int i;
    for (i = 0; i < m_AnimOverlay.Count(); i++)
    {
        layer[i] = MAX_OVERLAYS;
    }
    for (i = 0; i < m_AnimOverlay.Count(); i++)
    {
        CAnimationLayer &pLayer = m_AnimOverlay[i];
        if ((pLayer.m_flWeight > 0) && pLayer.IsActive() && pLayer.m_nOrder >= 0 && pLayer.m_nOrder < m_AnimOverlay.Count())
        {
            layer[pLayer.m_nOrder] = i;
        }
    }
    for (i = 0; i < m_AnimOverlay.Count(); i++)
    {
        if (layer[i] >= 0 && layer[i] < m_AnimOverlay.Count())
        {
            CAnimationLayer &pLayer = m_AnimOverlay[layer[i]];

            // UNDONE: Is it correct to use overlay weight for IK too?
            if (!remove_taunts || std::find(ignore_sequences.begin(), ignore_sequences.end(), pLayer.m_nSequence) == ignore_sequences.end())
                boneSetup.AccumulatePose(pos, q, pLayer.m_nSequence, pLayer.m_flCycle, pLayer.m_flWeight, g_GlobalVars->curtime, *m_pIk);
        }
    }

    if (m_pIk)
    {
        CIKContext auto_ik;
        QAngle angles = VectorToQAngle(re::C_BasePlayer::GetEyeAngles(ent));
        auto_ik.Init(pStudioHdr, angles, ent->GetAbsOrigin(), g_GlobalVars->curtime, 0, boneMask);
        boneSetup.CalcAutoplaySequences(pos, q, g_GlobalVars->curtime, &auto_ik);
    }
    else
    {
        boneSetup.CalcAutoplaySequences(pos, q, g_GlobalVars->curtime, NULL);
    }

    boneSetup.CalcBoneAdj(pos, q, &NET_FLOAT(ent, netvar.m_flEncodedController));
}

bool SetupBones(IClientEntity *ent, matrix3x4_t *pBoneToWorld, int boneMask)
{
    CStudioHdr *pStudioHdr = NET_VAR(ent, 0x880, CStudioHdr *);

    if (!pStudioHdr)
        return false;

    if (pBoneToWorld == nullptr)
    {
        pBoneToWorld = new matrix3x4_t[sizeof(matrix3x4_t) * MAXSTUDIOBONES];
    }

    int *entity_flags = (int *) ((uintptr_t) ent + 400);

    // EFL_SETTING_UP_BONES
    *entity_flags |= 1 << 3;

    Vector pos[MAXSTUDIOBONES];
    Quaternion q[MAXSTUDIOBONES];

    static uintptr_t m_pIK_offset = 0x568;

    Vector adjOrigin = ent->GetAbsOrigin();

    CIKContext **m_pIk = (reinterpret_cast<CIKContext **>(reinterpret_cast<uint64_t>(ent) + (m_pIK_offset)));
    QAngle angles = VectorToQAngle(re::C_BasePlayer::GetEyeAngles(ent));

    // One function seems to have pitch as it will do weird stuff with the bones, so we just do this as a fix
    QAngle angles2 = angles;
    angles2.x      = 0;
    angles2.z      = 0;

    if (*m_pIk)
    {
        // FIXME: pass this into Studio_BuildMatrices to skip transforms
        CBoneBitList boneComputed;
        // m_iIKCounter++;
        (*m_pIk)->Init(pStudioHdr, angles, adjOrigin, g_GlobalVars->curtime, 0 /*m_iIKCounter*/, boneMask);
        GetSkeleton(ent, pStudioHdr, pos, q, boneMask);

        (*m_pIk)->UpdateTargets(pos, q, pBoneToWorld, boneComputed);

        typedef void (*CalculateIKLocks_t)(IClientEntity *, float);
        static uintptr_t CalculateIKLocks_sig      = gSignatures.GetClientSignature("55 89 E5 57 56 53 81 EC EC 04 00 00");
        static CalculateIKLocks_t CalculateIKLocks = (CalculateIKLocks_t) CalculateIKLocks_sig;

        CalculateIKLocks(ent, g_GlobalVars->curtime);
        (*m_pIk)->SolveDependencies(pos, q, pBoneToWorld, boneComputed);
    }
    else
    {
        GetSkeleton(ent, pStudioHdr, pos, q, boneMask);
    }

    // m_flModelScale
    Studio_BuildMatrices(pStudioHdr, angles2, adjOrigin, pos, q, -1, NET_FLOAT(ent, netvar.m_flModelScale), // Scaling
                         pBoneToWorld, boneMask);

    // EFL_SETTING_UP_BONES
    *entity_flags &= ~(1 << 3);

    return true;
}
} // namespace setupbones_reconst
