
/*
 *
 * Useful debug tools for debugging cheat features and functions
 *
 */

#include "../util/logging.hpp"
#include "../util/catvars.hpp"
#include "../framework/gameticks.hpp"
#include "../framework/entitys.hpp"
#include "../framework/drawing.hpp"
#include "aimbot.hpp"

#include "debug.hpp"

// TODO, draw multipoint

namespace features::debug {

static CatEnum debug_menu({"debug"});

CatCommand debug_ent_dump("debug_entity_dump", [](std::vector<std::string>){
  CatLogger tmp_log("/tmp/entdump.tmp");
  g_CatLogging.log("Start Entity Info Dump---");
  tmp_log.log("Start Entity Info Dump---");
  // General ent info
  tmp_log.log("Entity Count: %i", GetEntityCount());
  // local
  auto local_ent = GetLocalPlayer();
  if (!local_ent)
    tmp_log.log("Local Entity: Null");
  else
    tmp_log.log("Local Entity:\n\tThirdperson: %i\n\tCamera: %i\n\tCamera Angles: %i",
      InThirdperson(local_ent), GetCamera(local_ent) != CatVector(), GetCameraAngle(local_ent) != CatVector());
  // other
  for (int i = 0; i < GetEntityCount(); i++) {
    auto ent = GetEntity(i);
    if (!ent)
      tmp_log.log("Entity %i: Null", i);
    else if (GetDormant(ent))
      tmp_log.log("Entity %i: Dormant: 1", i);
    else if (!GetAlive(ent))
      tmp_log.log("Entity %i: \n\tDormant: 0\n\tAlive: 0", i);
    else
      tmp_log.log("Entity %i: \n\tDormant: 0\n\tAlive: 1\n\tHealth: %i\n\t"
      "MaxHealth: %i\n\tTeam:%i\n\tType: %i\n\tName: %s\n\tOrigin: %i\n\t"
      "Collision: %i\n\tSteamId: %i",
      i, GetHealth(ent), GetMaxHealth(ent), GetTeam(ent), GetType(ent),
      GetName(ent), GetOrigin(ent) != CatVector(), GetCollision(ent) != CatBox(),
      GetSteamId(ent));
  }
  tmp_log.log("End Entity Info Dump---");
  g_CatLogging.log("End Entity Info Dump---");
});

// todo fov debug
static void WorldTick(){

}

CatVarBool multi_debug(debug_menu, "debug_multipoint", false, "Multipoint");

static void DrawTick(){

  // Multipoint shit, its pasted from aimbot so if u change that code, this will need to be remade to fix the math
  if (multi_debug) {
    for(int i = 0; i < GetEntityCount(); i++) {
      auto entity = GetEntity(i);
      if (!entity || GetDormant(entity) || !GetAlive(entity) || GetType(entity) != ETYPE_PLAYER) continue;

  		CatBox tmp_bone;
  		if (!GetBone(entity, EBone_head, tmp_bone)) continue;

      // So i dont need to do this everywhere :/
      auto draw_pt = [](CatVector in, CatVector4 color) {
        CatVector wts;
        if (!draw::WorldToScreen(in, wts)) return;
        draw::RectFilled(wts.x - 3, wts.y - 3, 6, 6, color);
        draw::Rect(wts.x - 4, wts.y - 4, 8, 8, colors::black);
      };

      // Draw multipoints
		  auto ratio = (tmp_bone * ((float)aimbot::multipoint_ratio * 0.01)) / aimbot::multipoint;
  		for (int i = 1; i <= aimbot::multipoint; i++)
  		  for (const auto ii : (ratio * i).GetPoints())
          draw_pt(ii, colors::white);

      // Draw center, and sides
      draw_pt(tmp_bone.GetCenter(), colors::red);
      draw_pt(tmp_bone.min, colors::red);
      draw_pt(tmp_bone.max, colors::red);
		}
  }
}

void Init() {
  drawmgr.REventDuring(DrawTick);
  wtickmgr.REventDuring(WorldTick);
}

}
