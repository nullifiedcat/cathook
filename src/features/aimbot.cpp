
/*
 *
 *	The cheats main aimbot system.
 *
 *
 *
 */

#include "../util/chrono.hpp"
#include "../framework/gameticks.hpp" // To run our stuff
#include "../framework/input.hpp" // to get userinput for aimkeys
#include "../framework/trace.hpp" // so we can vis check

#include "aimbot.hpp"

namespace features::aimbot {

// do the dew with "punchangles"

static CatEnum aimbot_menu({"Aimbot"}); // Menu locator for esp settings
static CatVarBool enabled(aimbot_menu, "ab", true, "Enable Aimbot", "Main aimbot switch");
// Target Selection
static CatEnum priority_mode_enum({"SMART", "FOV", "DISTANCE", "HEALTH"});
static CatVarEnum priority_mode(aimbot_menu, priority_mode_enum, "ab_prioritymode", 1, "Priority mode", "Priority mode.\nSMART: Basically Auto-Threat.\nFOV, DISTANCE, HEALTH are self-explainable.\nHEALTH picks the weakest enemy");
static CatVarFloat fov(aimbot_menu, "ab_fov", 0, "Aimbot FOV", "FOV range for aimbot to lock targets.", 180.0f);
static CatEnum teammates_enum({"ENEMY ONLY", "TEAMMATE ONLY", "BOTH"});
static CatVarEnum teammates(aimbot_menu, teammates_enum, "ab_teammates", 0, "Teammates", "Use to choose which team/s to target");
static CatVarBool target_lock(aimbot_menu, "ab_targetlock", false, "Target lock", "Once aimbot finds a target, it will continue to use that target untill that target is no longer valid");
// Aiming
static CatVarKey aimkey(aimbot_menu, "ab_aimkey", CATKEY_E, "Aimkey", "If an aimkey is set, aimbot only works while key is depressed.");
static CatVarBool autoshoot(aimbot_menu, "ab_autoshoot", true, "Auto-shoot", "Automaticly shoots when it can");
static CatEnum hitbox_mode_enum({"AUTO", "AUTO-HEAD", "AUTO-CLOSEST", "HEAD", "CENTER"});
static CatVarEnum hitbox_mode(aimbot_menu, hitbox_mode_enum, "ab_hitbox_mode", 0, "Hitbox Mode", "Hitbox selection mode\n"
																																																		 "AUTO: Automaticly chooses best hitbox\n"
																																																		 "AUTO-HEAD: Head is first priority, but will aim anywhere else if not possible\n"
																																																		 "AUTO-CLOSEST: Aims to the closest hitbox to your crosshair\n"
																																																		 "HEAD: Head only\n"
																																																		 "CENTER: Aims directly in the center of the entity");
static CatVarInt smooth_aim(aimbot_menu, "ab_smooth", 0, "Smooth Aim", "Smooths the aimbot");
static CatEnum silent_aim_enum({"OFF", "SNAPBACK", "MODULE"});
static CatVarEnum silent_aim(aimbot_menu, silent_aim_enum, "ab_silent", 0, "Silent aimbot", "SNAPBACK: Snaps the aimbot back after aiming\n"
																																																						"MODULE: Uses the modules own version of silent, if any");
CatVarInt multipoint(aimbot_menu, "ab_multipoint", 0, "Multipoint", "Amount of boxes to check, 0 = off\n NOTE: THIS IS EXTREMELY INTENSIVE, USE ONLY WHAT YOU NEED!", 5);
CatVarInt multipoint_ratio(aimbot_menu, "ab_multipoint_ratio", 86, "Multipoint Ratio", "Some games might have crappier lag comp than others, this shrinks the hitbox to correct for that.", 100);
static CatVarBool can_shoot(aimbot_menu, "ab_canshoot", true, "Can Shoot check", "Aimbot will only aim when the weapon can shoot\nDoesnt work well with legit configs if using weapons that shoot rappidly\nNEEDS MODULE SUPPORT");

// Hitbox selection

// Somewhere to store the auto-hitbox function
CMFunction<CatVector(CatEntity*)> GetAutoHitbox {[](CatEntity* entity){ return RetrieveAimpoint(entity, 3); }};

// A function to find a place to aim for on the target
CatVector RetrieveAimpoint(CatEntity* entity, int mode = hitbox_mode) {

	// A simple multi-point function, call it when you get bone center to get a point that hits. This does tracing for you.
	// First in pair is if it could get bone, second is if it could find a point
	// set trace to true if you want it tracing first point if multipoint is disabled
	// TODO: find a better way to impliment other than using as a drop-in for GetBone
 	auto GetMultiBone = [&](int bone, CatVector& out, bool trace = false) -> std::pair<bool, bool> {
		auto camera = GetCamera(GetLocalPlayer()); // we can assume we have a local ent at this point
		// Normal bone stuff
		CatBox tmp_bone;
		if (!GetBone(entity, bone, tmp_bone)) return {false, false};

		// If we arent using multi-point and we arent tracing first, we can just return it
		CatVector center = tmp_bone.GetCenter();
		if ((!multipoint && !trace) || trace::trace_entity(entity, camera, center)) {
			out = center;
			return {true, true};
		}

		// First we shrink the box to try to avoid latency issues, we do this once to save the little cycles we have
		// then we devide that by how many times we will recurse to get the ratio
		auto ratio = (tmp_bone * ((float)multipoint_ratio * 0.01)) / multipoint;
		for (int i = 1; i <= multipoint; i++) {
			// then we multiply it using the ratio to expand the point back to the size we want to check
			auto expanded = ratio * i;
			// Try to find a point that fits
			for (const auto ii : expanded.GetPoints()) {
				if (trace::trace_entity(entity, camera, ii)) {
					out = ii;
					return {true, true};
				}
			}
		}

		// We didnt get any points ;( but we can still return the first, which is already set, and return trace as false
		return {true, false};
	};

	// Get our best bone
	switch(mode) {
	case 0: { // AUTO
		return GetAutoHitbox(entity);
	}
	case 1: { // AUTO-HEAD
		// Head is first bone, should be fine to iterate through them
		for (int i = 0; i < EBone_count; i++) {
			// Get our bone
			CatVector tmp;
			if (!GetMultiBone(i, tmp, true).first) continue;
			return tmp;
		}
		break;
	}
	case 2: { // AUTO-CLOSEST
		// We need this for fov checks
		auto local_ent = GetLocalPlayer();
		auto camera_ang = GetCameraAngle(local_ent);
		auto camera = GetCamera(local_ent);
		// Book-keepers for the best one we have found
		CatVector closest = CatVector();
		float closest_fov = 360;
		for (int i = 0; i < EBone_count; i++) {
			// Get our bone
			CatVector tmp;
			if (!GetMultiBone(i, tmp).first) continue;
			// Get FOV
			float fov = util::GetFov(camera_ang, camera, tmp);
			// Check if fov is lower than our current best
			if (fov > closest_fov) continue;
			// Vis check
			if (!trace::trace_entity(entity, camera, tmp))
				continue;
			// Set the new current best
			closest = tmp;
			closest_fov = fov;
		}
		// Check if we have anything set, then return if true
		if (closest != CatVector())
			return closest;
		break;
	}
	case 3: { // HEAD
		CatVector tmp;
		if (GetMultiBone(EBone_head, tmp).first) return tmp; // if we couldnt get a bone, we let collision take over
		break;
	}
	}

	// Center fallback, uses center of collision box

	// Check if collision box is set, TODO: multipoint collision?
	auto coll = GetCollision(entity);
	if (coll != CatBox())
		// We can use the center collision for an aimpoint
		return coll.GetCenter();

	// Without anything else, all we can use is the origin
	return GetOrigin(entity);
}

// Target Selection

// For modules to add their target selection stuff
std::vector<CMFunction<bool(CatEntity*)>> TargetSelectionModule;

// A second check to determine whether a target is good enough to be aimed at
static std::pair<bool, CatVector> IsTargetGood(CatEntity* entity) {
	auto ret = std::make_pair(false, CatVector());
	if (!entity || GetDormant(entity)) return ret;

	// Local player check
	auto local_ent = GetLocalPlayer(); // we use this below
	if (!local_ent || (CatEntity*)local_ent == entity) return ret;

	// Type
	auto type = GetType(entity);
	if (type != ETYPE_PLAYER && type != ETYPE_OTHERHOSTILE) return ret;

	// Dead
	if (!GetAlive(entity)) return ret;

	// Teammates
	auto team = GetEnemy(entity);
	if (teammates != 2 && ((teammates == 0) ? !team : team)) return ret;

	// Do the custom stuff
	for (auto tmp : TargetSelectionModule)
		if (!tmp(entity)) return ret;

	// Get our best Aimpoint
	CatVector aimpoint = RetrieveAimpoint(entity);

	// Fov check
	if (fov > 0.0f && util::GetFov(GetCameraAngle(local_ent), GetCamera(local_ent), aimpoint) > fov) return std::make_pair(false, aimpoint);

	// Vis check
	if (!trace::trace_entity(entity, GetCamera(local_ent), aimpoint))
		return std::make_pair(false, aimpoint);

	// Hey look! Target passed all checks
	return std::make_pair(true, aimpoint);
}

static CatEntity* last_target = nullptr; // for target lock
// Function to find a suitable target
static std::pair<CatEntity*, CatVector> RetrieveBestTarget() {

	// If target lock is enabled, check last entity.
	if (target_lock && last_target) {
		auto tmp = IsTargetGood(last_target);
		if (tmp.first)
			return std::make_pair(last_target, tmp.second);
	}

	// Book keepers for highest target
	std::pair<CatEntity*, CatVector> ret;
	float highest_score = -1024;

	auto local_ent = GetLocalPlayer();
	if (!local_ent) return ret;

	// Loop through all entitys
	for (int i = 0; i < GetEntityCount(); i++) {
		auto entity = GetEntity(i);
		if (!entity) continue;

		// Check whether or not we can target the ent
		auto tmp = IsTargetGood(entity);
		if (!tmp.first) continue;

		// Get score based on priority mode
		float score = 0;
		switch (priority_mode) {
		case 0: // SMART Priority
			//score = 0; break; // TODO, fix
		case 1: // Fov Priority
			score = 180.0f - util::GetFov(GetCameraAngle(local_ent), GetCamera(local_ent), tmp.second); break;
		case 2: // Distance priority
			score = 4096.0f - GetDistance(entity); break;
		case 3: // Health Priority
			score = 1024.0f - GetHealth(entity);
		}

		// Compare the top score to our current ents score
		if (score > highest_score) {
			ret = std::make_pair(entity, tmp.second);
			highest_score = score;
		}
	}

	return ret;
}
// A check to determine whether the local player should aimbot
static bool ShouldAim() {

	// It would be prefered to have a local ent before we shoot
	auto local_ent = (CatEntity*)GetLocalPlayer();
	if (!local_ent || GetDormant(local_ent)) return false;
	// Alive check
	if (!GetAlive(local_ent)) return false;
	// Aimkey
	if (!aimkey.Depressed()) return false;

	return true;
}

// Externed entity to highlight color
CatEntity* highlight_target = nullptr;

// Used so a module can return whether weapon can shoot
CMFunction<bool()> CanShoot {[]() -> bool {return true;}};

// The main "loop" of the aimbot.
static void WorldTick() {

	// Main enabled check
	if (!enabled) { last_target = nullptr; highlight_target = nullptr; return; }

	// Get local ent for use below
	auto local_ent = GetLocalPlayer();
	if (!local_ent) { last_target = nullptr; highlight_target = nullptr; return; }

	// Snapback Silent Info
	static std::tuple<int, CatVector, CatTimer> snap_info;
	// Return for snapback
	auto preRet = [&](){
		if (silent_aim == 1) {
			if (std::get<0>(snap_info)) {
				auto delta = GetCameraAngle(local_ent) + std::get<1>(snap_info);
				SetCameraAngle(local_ent, util::ClampAngles(delta));
			}
			std::get<0>(snap_info) = false;
		}
		last_target = nullptr;
	};

	// Attempt to get a target and test if it exist
	auto target = RetrieveBestTarget();
	highlight_target = target.first;
	if (!target.first) { preRet(); return; }

	// Check if our local player is ready to aimbot
	if (!ShouldAim()) { preRet(); return; }

	// Get camera so we wont need to again
	const auto camera_ang = GetCameraAngle(local_ent);

	// Snapback Time Reset
	if (silent_aim == 1 && std::get<0>(snap_info)) {
		if (std::get<0>(snap_info) == 1 && std::get<2>(snap_info).ResetAfter(std::chrono::milliseconds(100))) {
			auto delta = camera_ang + std::get<1>(snap_info);
			SetCameraAngle(local_ent, util::ClampAngles(delta));
			std::get<0>(snap_info) = 2;
		}
		return;
	}

	// Set our last target
	last_target = target.first;

	// Do smoothaim
	if (smooth_aim > 0) {

		// TODO, Smooth is only somewhat fixed, it still needs a change to fix the crossing of the y axis (-180, 180)

		auto camera = GetCamera(local_ent);
		auto angles = util::VectorAngles(camera, target.second);
		auto delta = util::GetAngleDifference(camera_ang, angles);

		// Pitch, If our camera pitch is more than our target pitch, we should add to lower that value, and vise versa for camera being lower
		float p_move_ammt = delta.x / pow(smooth_aim, 1.5);
		angles.x = (camera_ang.x > angles.x) ? (camera_ang.x - p_move_ammt) : (camera_ang.x + p_move_ammt);

		// Yaw, same as above but If we go across -180 to 180, we do some changes
		float y_move_ammt = delta.y / pow(smooth_aim, 1.5);
		angles.y = (camera_ang.y > angles.y || (camera_ang.y > 90 && angles.y < -90)) ?
			(camera_ang.y - y_move_ammt) : (camera_ang.y + y_move_ammt);

		// Clamp as we changed some values
		util::ClampAngles(angles);

		// Aim here as silent wont work with smooth
		SetCameraAngle(local_ent, angles);

		// Slowaim Autoshoot, basicly we recreate an extremly simple triggerbot here
		if (autoshoot) {
			// The further the terget gets from us, we want to make the allowed fov lower
			if (util::GetFov(camera_ang, camera, target.second) - camera.DistTo(target.second) / 32 < 5)
				Attack(local_ent);
		}
	}
	else {
		// Autoshoot
		if (autoshoot)
			Attack(local_ent);
		// Check weapon time, we only want to aim when the weapon can shoot
		// Get angles and Aim at player
		auto aim_angles = util::VectorAngles(GetCamera(local_ent), target.second);
		switch(silent_aim){
		case 0: // OFF
			SetCameraAngle(local_ent, aim_angles); break;
		case 1: { // SNAPBACK
			// Setup the snap
			if (!std::get<0>(snap_info)) {
				std::get<0>(snap_info) = true;
				std::get<1>(snap_info) = camera_ang - aim_angles; // we use a delta in case player looks around
				std::get<2>(snap_info).Reset();
			}
			// Set angles
			SetCameraAngle(local_ent, aim_angles);
			break;
		}
		case 2: // MODULE
			 SetSilentCameraAngle(local_ent, aim_angles); break;
		}
	}
}

static void DrawTick() {

}

void Init() {
	wtickmgr.REventDuring(WorldTick);
	drawmgr.REventDuring(DrawTick);
}

}
