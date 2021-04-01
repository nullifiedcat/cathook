#include "CServerTools.hpp"

IServerEntity *CServerTools::GetIServerEntity(IClientEntity *pClientEntity)
{
    typedef IServerEntity *(*GetIServerEntity_t)(CServerTools *, IClientEntity *);
    static uintptr_t GetIServerEntity_sig         = gSignatures.GetServerSignature("55 89 E5 53 83 EC 14 8B 45 ? 85 C0 74 ? 8B 10");
    static GetIServerEntity_t GetIServerEntity_fn = (GetIServerEntity_t) GetIServerEntity_sig;
    return GetIServerEntity_fn(this, pClientEntity);
}
