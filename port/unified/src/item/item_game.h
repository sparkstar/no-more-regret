/* Written by port/tools/make_unified_values.py from regret/src/item/item_game.h and remorse/src/item/item_game.h: the
 * one program's run-time values; do not edit. */

#ifndef PORT_ITEM_GAME_H
#define PORT_ITEM_GAME_H
#include <span>
#include <array>
#include "common.h"

extern std::span<const u16> CRATE_SHAPES;
extern bool ON_SCREEN_WHOLE_HEIGHT;
extern bool DEAD_NPCS_RESTART;
extern std::span<const StrongerWeapon> STRONGER_WEAPONS;
extern i16 DEFAULT_STRONGER_WEAPON;
extern bool EGG_MARKS_ARMED;
extern bool WEAPONS_ONLY_FOR_CONTAINERS;
#endif
