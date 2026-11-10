/* Written by port/tools/make_unified_values.py from regret/src/game/crusader_game.h and
 * remorse/src/game/crusader_game.h: the one program's run-time values; do not edit. */

#ifndef PORT_CRUSADER_GAME_H
#define PORT_CRUSADER_GAME_H
#include <span>
#include <array>
#include "common.h"
#include "animprim.h"

extern std::span<const u16> ROBOT_SHAPES;
extern u16 DEATH_SOUND;
extern bool DEATH_CLEARS_STASIS;
extern std::span<const i16> KEPT_ANIMS;
extern bool SMALL_WEAPON_ANIMS;
extern bool WEAK_WEAPONS_TAKE_TYPE_DAMAGE;
extern bool NPCS_AIM_AT_ATTACK_TARGET;
extern bool TAKE_HIT_NO_REGRET;
extern i16 HURL_X_NO_DIRECTION;
extern i16 HURL_Y_NO_DIRECTION;
#endif
