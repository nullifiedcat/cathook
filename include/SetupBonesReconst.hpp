#pragma once

#include "sdk.hpp"

namespace setupbones_reconst
{
bool SetupBones(IClientEntity *ent, matrix3x4_t *pBoneToWorld, int boneMask);
}
