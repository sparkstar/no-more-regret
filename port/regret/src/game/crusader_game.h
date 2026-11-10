/* No Regret's values of the hits (common/src/game/crusader.cpp). */
#ifndef PORT_CRUSADER_GAME_H
#define PORT_CRUSADER_GAME_H
#include <array>
#include "common.h"
#include "animprim.h"

/* The robots' shapes, which are not organic (isOrganic). */
inline constexpr std::array<u16, 14> ROBOT_SHAPES = {0x4c8, 0x338, 0x45d, 0x2cb, 0x4e6, 0x383, 0x385,
                                                     0x1cd, 0x5e6, 0x319, 0x30c, 0x5e2, 0x5b1, 0x5ba};
/* The Silencer's death sound (DeathSilence::run), and the stasis ends. */
inline constexpr u16 DEATH_SOUND = 0x194;
inline constexpr bool DEATH_CLEARS_STASIS = true;
/* The animations the Silencer's enemies keep (npcAnim), to -1. */
inline constexpr std::array<i16, 8> KEPT_ANIMS = {
    ANIM_COMBAT_STAND_SMALL, ANIM_UNREADY_SMALL, ANIM_READY_SMALL,    ANIM_FIRE_SMALL,
    ANIM_ADVANCE_SMALL,      ANIM_RETREAT_SMALL, ANIM_STOP_RUN_SMALL, -1};
/* With a small weapon or none, an animation of npcAnim's pairs is played as
 * its pair (npcAnim). */
inline constexpr bool SMALL_WEAPON_ANIMS = true;
/* An NPC's shot whose weapon gives at most 1 damage takes its fire type's
 * damage, and an NPC's aimed shot aims at its attack target (No
 * Remorse's at the controlled NPC) (fireWeapon). */
inline constexpr bool WEAK_WEAPONS_TAKE_TYPE_DAMAGE = true;
inline constexpr bool NPCS_AIM_AT_ATTACK_TARGET = true;
/* A hit is taken by No Regret's rules (takeHit): its boss's shield sparks, the shape 5D6h's fall, its gory deaths and
 * their thresholds, its hurt sounds. */
inline constexpr bool TAKE_HIT_NO_REGRET = true;
/* How far a hit with no direction (NO_DIRECTION: the hit at the thing's x and y) pushes a thing on each axis
 * (thingTakesHit): the words the original reads past its tables of the 16 directions (hurlX 1480:2d7e, hurlY
 * 1480:2d9e), as its data segment lays them out. x's is the first of hurlY; y's is finalHit's byte (1480:2dbe) and
 * the low byte of the first death sound of men (88h), 8800h: finalHit is 0 whenever a thing is hurled (it is 1 only
 * for the end's hit on the Silencer, an NPC). */
inline constexpr i16 HURL_X_NO_DIRECTION = -2;
inline constexpr i16 HURL_Y_NO_DIRECTION = static_cast<i16>(0x8800);
#endif
