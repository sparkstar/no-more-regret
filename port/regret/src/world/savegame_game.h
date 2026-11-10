/* No Regret's values of the saved games (common/src/world/savegame.cpp). */
#ifndef PORT_SAVEGAME_GAME_H
#define PORT_SAVEGAME_GAME_H
#include <array>
#include "savegame.h"

/* The entries of a saved game's FLEX index, which the processes' objects
 * (the kernel's, from 100) fill: the original's, whose saves the original
 * reads. */
inline constexpr i16 SAVE_FLEX_ENTRIES = 0xc00;
/* The objects the game saves after the alert: none. */
inline constexpr std::array<SavedObject, 0> GAME_SAVED_OBJECTS{};
#endif
