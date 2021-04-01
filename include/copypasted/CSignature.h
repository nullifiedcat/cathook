#pragma once

// Copypasted from Darkstorm 2015 linux base

//#include "SDK.h"
#include <stdint.h>
#include "sharedobj.hpp"
#include <elf.h>

namespace CSignature_space
{
struct SharedObjStorage
{
    bool inited;
    void *module;
    link_map *moduleMap;
    int textOffset;
    int textSize;
    SharedObjStorage()
    {
    }
    SharedObjStorage(void *mod, link_map *modmap, int txtoffset, int txtsize) : module{ mod }, moduleMap{ modmap }, textOffset{ txtoffset }, textSize{ txtsize }
    {
    }
};

enum SharedObjects
{
    client = 0,
    engine,
    steamapi,
    vstd,
    launcher,
    server,
    // Whatever you do, keep this one last
    entry_count
};
} // namespace CSignature_space

class CSignature
{
public:
    static uintptr_t dwFindPattern(uintptr_t dwAddress, uintptr_t dwLength, const char *szPattern);
    static void *GetModuleHandleSafe(const char *pszModuleName);
    static uintptr_t GetSignature(const char *chPattern, sharedobj::SharedObject &obj, int idx);
    static uintptr_t GetClientSignature(const char *chPattern);
    static uintptr_t GetEngineSignature(const char *chPattern);
    static uintptr_t GetLauncherSignature(const char *chaPattern);
    static uintptr_t GetSteamAPISignature(const char *chPattern);
    static uintptr_t GetVstdSignature(const char *chPattern);
    static uintptr_t GetServerSignature(const char *chPattern);
};

extern CSignature gSignatures;
