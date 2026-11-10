/* Written by port/tools/make_unified_values.py from regret/src/game/explode_game.h and remorse/src/game/explode_game.h:
 * the one program's run-time values; do not edit. */

#ifndef PORT_EXPLODE_GAME_H
#define PORT_EXPLODE_GAME_H
#include <span>
#include <array>
#include "common.h"

extern bool EXPLOSION_ALERTS;
extern bool EXPLOSION_SOUND_AT_ITEM;
extern std::span<const u16> GRENADE_BLASTS;
extern std::span<const u16> ROCKET_BLASTS;
extern bool MORE_HIT_SOUNDS;
#endif
