/* No Remorse's values of the hits (common/src/game/crusader.cpp). */
#ifndef PORT_CRUSADER_GAME_H
#define PORT_CRUSADER_GAME_H
#include <array>
#include "common.h"
#include "animprim.h"

/* The robots' shapes, which are not organic (isOrganic). */
inline constexpr std::array<u16, 7> ROBOT_SHAPES = {0x4c8, 0x338, 0x45d, 0x2cb, 0x4e6, 0x383, 0x385};
/* The Silencer's death sound (DeathSilence::run), and the stasis stays (No Regret's ends it). */
inline constexpr u16 DEATH_SOUND = 9;
inline constexpr bool DEATH_CLEARS_STASIS = false;
/* The animations the Silencer's enemies keep (npcAnim), to -1 (No Regret's has the small stop of a run too). */
inline constexpr std::array<i16, 7> KEPT_ANIMS = {ANIM_COMBAT_STAND_SMALL,
                                                  ANIM_UNREADY_SMALL,
                                                  ANIM_READY_SMALL,
                                                  ANIM_FIRE_SMALL,
                                                  ANIM_ADVANCE_SMALL,
                                                  ANIM_RETREAT_SMALL,
                                                  -1};
/* With a small weapon or none, an animation of npcAnim's pairs is played as
 * its pair: No Regret's; No Remorse's keeps it (npcAnim). */
inline constexpr bool SMALL_WEAPON_ANIMS = false;
/* An NPC's shot whose weapon gives at most 1 damage takes its fire type's
 * damage: No Regret's, and an NPC's aimed shot aims at its attack target (No
 * Remorse's at the controlled NPC); both No Regret's (fireWeapon). */
inline constexpr bool WEAK_WEAPONS_TAKE_TYPE_DAMAGE = false;
inline constexpr bool NPCS_AIM_AT_ATTACK_TARGET = false;
/* A hit is taken by No Regret's rules (takeHit): not No Remorse's, whose boss teleports with its shield, and whose
 * deaths and thresholds are its own. */
inline constexpr bool TAKE_HIT_NO_REGRET = false;
/* How far a hit with no direction (NO_DIRECTION: the hit at the thing's x and y) pushes a thing on each axis
 * (thingTakesHit): the words the original reads past its tables of the 16 directions (hurlX 1478:266e, hurlY
 * 1478:268e), as its data segment lays them out. x's is the first of hurlY; y's is the first death sound of men (88h,
 * 1478:26ae). */
inline constexpr i16 HURL_X_NO_DIRECTION = -2;
inline constexpr i16 HURL_Y_NO_DIRECTION = 0x88;
#endif
