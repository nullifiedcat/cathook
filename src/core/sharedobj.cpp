/*
 * sharedobj.cpp
 *
 *  Created on: Oct 3, 2016
 *      Author: nullifiedcat
 */

#include "common.hpp"

namespace sharedobj
{

bool LocateSharedObject(std::string &name, std::string &out_full_path)
{
    FILE *proc_maps = fopen("/proc/self/maps", "r");
    if (proc_maps == nullptr)
    {
        return false;
    }
    char buffer[512];
    while (fgets(buffer, 511, proc_maps))
    {
        char *path     = strchr(buffer, '/');
        char *filename = strrchr(buffer, '/') + 1;
        if (not path or not filename)
            continue;
        if (!strncmp(name.c_str(), filename, name.length()))
        {
            out_full_path.assign(path);
            out_full_path.resize(out_full_path.size() - 1);
            fclose(proc_maps);
            return true;
        }
    }
    fclose(proc_maps);
    return false;
}

SharedObject::SharedObject(const char *_file, bool _factory) : file(_file), path("unassigned"), factory(_factory)
{
    constructed = true;
}

void SharedObject::Load()
{
    while (not LocateSharedObject(file, path))
    {
        sleep(1);
    }
    while (!(lmap = (link_map *) dlopen(path.c_str(), RTLD_NOLOAD)))
    {
        sleep(1);
        char *error = dlerror();
        if (error)
        {
            logging::Info("DLERROR: %s", error);
        }
    }
    logging::Info("Shared object %s loaded at 0x%08x", basename(lmap->l_name), lmap->l_addr);
    if (factory)
    {
        fptr = reinterpret_cast<fn_CreateInterface_t>(dlsym(lmap, "CreateInterface"));
        if (!this->fptr)
        {
            logging::Info("Failed to create interface factory for %s", basename(lmap->l_name));
        }
    }
}

void SharedObject::Unload()
{
    if (lmap)
    {
        dlclose(lmap);
        lmap = nullptr;
    }
    fptr = nullptr;
}

char *SharedObject::Pointer(uintptr_t offset) const
{
    if (this->lmap != nullptr)
    {
        return reinterpret_cast<char *>(uintptr_t(lmap->l_addr) + offset);
    }
    else
    {
        return nullptr;
    }
}

void *SharedObject::CreateInterface(const std::string &interface)
{
    return (void *) (fptr(interface.c_str(), nullptr));
}

void LoadLauncher()
{
    try
    {
        launcher().Load();
    }
    catch (std::exception &ex)
    {
        logging::Info("Exception: %s", ex.what());
    }
}

void LoadEarlyObjects()
{
    try
    {
        engine().Load();
        filesystem_stdio().Load();
        tier0().Load();
        materialsystem().Load();
    }
    catch (std::exception &ex)
    {
        logging::Info("Exception: %s", ex.what());
    }
}

void LoadAllSharedObjects()
{
    try
    {
        steamclient().Load();
        client().Load();
        server().Load();
        steamapi().Load();
        vstdlib().Load();
        inputsystem().Load();
        datacache().Load();
        vgui2().Load();
#if ENABLE_VISUALS
        vguimatsurface().Load();
        studiorender().Load();
        libsdl().Load();
#endif
    }
    catch (std::exception &ex)
    {
        logging::Info("Exception: %s", ex.what());
    }
}

void UnloadAllSharedObjects()
{
    steamclient().Unload();
    client().Unload();
    steamapi().Unload();
    vstdlib().Unload();
    inputsystem().Unload();
    datacache().Unload();
    vgui2().Unload();
#if ENABLE_VISUALS
    vguimatsurface().Unload();
    studiorender().Unload();
    libsdl().Unload();
#endif
    launcher().Unload();
    engine().Unload();
    filesystem_stdio().Unload();
    tier0().Unload();
    materialsystem().Unload();
}

SharedObject &steamclient()
{
    static SharedObject obj("steamclient.so", true);
    return obj;
}
SharedObject &steamapi()
{
    static SharedObject obj("libsteam_api.so", false);
    return obj;
}
SharedObject &client()
{
    static SharedObject obj("client.so", true);
    return obj;
}
SharedObject &engine()
{
    static SharedObject obj("engine.so", true);
    return obj;
}
SharedObject &launcher()
{
    static SharedObject obj("launcher.so", true);
    return obj;
}
SharedObject &vstdlib()
{
    static SharedObject obj("libvstdlib.so", true);
    return obj;
}
SharedObject &tier0()
{
    static SharedObject obj("libtier0.so", false);
    return obj;
}
SharedObject &inputsystem()
{
    static SharedObject obj("inputsystem.so", true);
    return obj;
}
SharedObject &materialsystem()
{
    static SharedObject obj("materialsystem.so", true);
    return obj;
}

SharedObject &filesystem_stdio()
{
    static SharedObject obj("filesystem_stdio.so", true);
    return obj;
}
SharedObject &datacache()
{
    static SharedObject obj("datacache.so", true);
    return obj;
}

SharedObject &vgui2()
{
    static SharedObject obj("vgui2.so", true);
    return obj;
}

SharedObject &server()
{
    static SharedObject obj("server.so", true);
    return obj;
}

#if ENABLE_VISUALS
SharedObject &vguimatsurface()
{
    static SharedObject obj("vguimatsurface.so", true);
    return obj;
}
SharedObject &studiorender()
{
    static SharedObject obj("studiorender.so", true);
    return obj;
}
SharedObject &libsdl()
{
    static SharedObject obj("libSDL2-2.0.so.0", false);
    return obj;
}

#endif
} // namespace sharedobj
