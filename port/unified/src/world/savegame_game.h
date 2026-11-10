/* Written by port/tools/make_unified_values.py from regret/src/world/savegame_game.h and
 * remorse/src/world/savegame_game.h: the one program's run-time values; do not edit. */

#ifndef PORT_SAVEGAME_GAME_H
#define PORT_SAVEGAME_GAME_H
#include <span>
#include <array>
#include "savegame.h"
#include "wezzy.h"

extern i16 SAVE_FLEX_ENTRIES;
extern std::span<const SavedObject> GAME_SAVED_OBJECTS;
#endif
