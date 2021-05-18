/*
 * sharedobj.h
 *
 *  Created on: Oct 3, 2016
 *      Author: nullifiedcat
 */

#pragma once

#include <string>
#include <vector>
#include "config.h"

struct link_map;
typedef void *(*fn_CreateInterface_t)(const char *, int *);

namespace sharedobj
{

bool LocateSharedObject(std::string &name, std::string &out_full_path);

class SharedObject
{
public:
    SharedObject(const char *_file, bool _factory);
    void Load();
    void Unload();
    char *Pointer(uintptr_t offset) const;
    void *CreateInterface(const std::string &interface);

public:
    std::string file;
    std::string path;
    bool factory{ false };
    bool constructed{ false };

    fn_CreateInterface_t fptr{ nullptr };
    link_map *lmap{ nullptr };
};

SharedObject &serverbrowser();
SharedObject &steamclient();
SharedObject &client();
SharedObject &engine();
SharedObject &launcher();
SharedObject &steamapi();
SharedObject &vstdlib();
SharedObject &tier0();
SharedObject &inputsystem();
SharedObject &materialsystem();
SharedObject &filesystem_stdio();
SharedObject &datacache();
SharedObject &vgui2();
SharedObject &server();
#if ENABLE_VISUALS
SharedObject &vguimatsurface();
SharedObject &studiorender();
SharedObject &libsdl();
#endif

void LoadLauncher();
void LoadEarlyObjects();
void LoadAllSharedObjects();
void UnloadAllSharedObjects();
} // namespace sharedobj
