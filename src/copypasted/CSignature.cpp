// Replace darkstorm stuff with my stuff
//#include "SDK.h"

#include "common.hpp"

// module should be a pointer to the base of an elf32 module
// this is not the value returned by dlopen (which returns an opaque handle to
// the module) the best method to get this address is with fopen() and mmap()
// (check the GetClientSignature() and GetEngineSignature() for an example)
Elf32_Shdr *getSectionHeader(void *module, const char *sectionName)
{
    // we need to get the modules actual address from the handle

    Elf32_Ehdr *ehdr = (Elf32_Ehdr *) module;
    Elf32_Shdr *shdr = (Elf32_Shdr *) ((unsigned) module + ehdr->e_shoff);

    Elf32_Shdr *strhdr = &shdr[ehdr->e_shstrndx];

    char *strtab        = NULL;
    unsigned strtabSize = 0;
    if (strhdr != NULL && strhdr->sh_type == 3)
    {
        strtab = (char *) ((unsigned) module + strhdr->sh_offset);

        if (strtab == NULL)
            // Log::Fatal("String table was NULL!");
            logging::Info("String table was NULL!");
        strtabSize = strhdr->sh_size;
    }
    else
    {
        // Log::Fatal("String table header was corrupted!");
        logging::Info("String table header was corrupted!");
    }

    for (int i = 0; i < ehdr->e_shnum; i++)
    {
        Elf32_Shdr *hdr = &shdr[i];
        if (hdr && hdr->sh_name < strtabSize)
        {
            if (!strcmp(strtab + hdr->sh_name, sectionName))
                return hdr;
        }
    }
    return 0;
}
bool InRange(char x, char a, char b)
{
    return x >= a && x <= b;
}

int GetBits(char x)
{
    if (InRange((char) (x & (~0x20)), 'A', 'F'))
    {
        return (x & (~0x20)) - 'A' + 0xa;
    }
    else if (InRange(x, '0', '9'))
    {
        return x - '0';
    }

    return 0;
}
int GetBytes(const char *x)
{
    return GetBits(x[0]) << 4 | GetBits(x[1]);
}

/* Shoutouts
 * To
 * Marc3842h
 * for
 * the
 * way
 * better
 * sigscan
 */
uintptr_t CSignature::dwFindPattern(uintptr_t dwAddress, uintptr_t dwLength, const char *szPattern)
{
    const char *pattern  = szPattern;
    uintptr_t firstMatch = 0;

    uintptr_t start = dwAddress;
    uintptr_t end   = dwLength;

    for (uintptr_t pos = start; pos < end; pos++)
    {
        if (*pattern == 0)
            return firstMatch;

        const uint8_t currentPattern = *reinterpret_cast<const uint8_t *>(pattern);
        const uint8_t currentMemory  = *reinterpret_cast<const uint8_t *>(pos);

        if (currentPattern == '\?' || currentMemory == GetBytes(pattern))
        {
            if (firstMatch == 0)
                firstMatch = pos;

            if (pattern[2] == 0)
            {
                logging::Info("Found pattern \"%s\" at 0x%08X.", szPattern, firstMatch);
                return firstMatch;
            }

            pattern += currentPattern != '\?' ? 3 : 2;
        }
        else
        {
            pattern    = szPattern;
            firstMatch = 0;
        }
    }
    logging::Info("THIS IS SERIOUS: Could not locate signature: "
                  "\n============\n\"%s\"\n============",
                  szPattern);
    return 0;
}
/*uintptr_t CSignature::dwFindPattern(uintptr_t dwAddress, uintptr_t dwLength,
                                    const char *szPattern)
{
    const char *pat      = szPattern;
    uintptr_t firstMatch = NULL;
    for (uintptr_t pCur = dwAddress; pCur < dwLength; pCur++)
    {
        if (!*pat)
            return firstMatch;
        if (*pat == '\?' || *(uint8_t *) pCur == getByte(pat))
        {
            if (!firstMatch)
                firstMatch = pCur;
            if (!pat[2])
                return firstMatch;
            if (*pat == '\?')
                pat += 2;
            else
                pat += 3;
        }
        else
        {
            pat        = szPattern;
            firstMatch = 0;
        }
    }

    logging::Info("THIS IS SERIOUS: Could not locate signature: "
                  "\n============\n\"%s\"\n============",
                  szPattern);

    return NULL;
}*/
//===================================================================================
void *CSignature::GetModuleHandleSafe(const char *pszModuleName)
{
    void *moduleHandle = NULL;

    do
    {
        moduleHandle = dlopen(pszModuleName, RTLD_NOW);
        usleep(1);
    } while (moduleHandle == NULL);

    return moduleHandle;
}

static CSignature_space::SharedObjStorage objects[CSignature_space::entry_count];

uintptr_t CSignature::GetSignature(const char *chPattern, sharedobj::SharedObject &obj, int idx)
{
    // we need to do this becuase (i assume that) under the hood, dlopen only
    // loads up the sections that it needs into memory, meaning that we cannot
    // get the string table from the module.

    auto &object = objects[idx];
    if (!object.inited)
    {
        int fd       = open(obj.path.c_str(), O_RDONLY);
        void *module = mmap(NULL, lseek(fd, 0, SEEK_END), PROT_READ, MAP_SHARED, fd, 0);
        if ((unsigned) module == 0xffffffff)
            return NULL;
        link_map *moduleMap = obj.lmap;

        // static void *module = (void *)moduleMap->l_addr;

        Elf32_Shdr *textHeader = getSectionHeader(module, ".text");

        int textOffset = textHeader->sh_offset;

        int textSize = textHeader->sh_size;

        object        = CSignature_space::SharedObjStorage(module, moduleMap, textOffset, textSize);
        object.inited = true;
    }

    // we need to remap the address that we got from the pattern search from our
    // mapped file to the actual memory we do this by rebasing the address
    // (subbing the mmapped one and replacing it with the dlopened one.
    uintptr_t patr = dwFindPattern(((uintptr_t) object.module) + object.textOffset, ((uintptr_t) object.module) + object.textOffset + object.textSize, chPattern);
    if (!patr)
        return NULL;
    return patr - (uintptr_t)(object.module) + object.moduleMap->l_addr;
}
//===================================================================================
uintptr_t CSignature::GetClientSignature(const char *chPattern)
{
    return GetSignature(chPattern, sharedobj::client(), CSignature_space::client);
}
//===================================================================================
uintptr_t CSignature::GetEngineSignature(const char *chPattern)
{
    return GetSignature(chPattern, sharedobj::engine(), CSignature_space::engine);
}
//===================================================================================
uintptr_t CSignature::GetLauncherSignature(const char *chPattern)
{
    return GetSignature(chPattern, sharedobj::launcher(), CSignature_space::launcher);
}
//===================================================================================
uintptr_t CSignature::GetSteamAPISignature(const char *chPattern)
{
    return GetSignature(chPattern, sharedobj::steamapi(), CSignature_space::steamapi);
}
//===================================================================================
uintptr_t CSignature::GetVstdSignature(const char *chPattern)
{
    return GetSignature(chPattern, sharedobj::vstdlib(), CSignature_space::vstd);
}
//===================================================================================
uintptr_t CSignature::GetServerSignature(const char *chPattern)
{
    return GetSignature(chPattern, sharedobj::server(), CSignature_space::server);
}

CSignature gSignatures;
