/* No Remorse's values of the load and save gumps (common/src/gump/loadsave.h). */
#ifndef PORT_LOADSAVE_GAME_H
#define PORT_LOADSAVE_GAME_H
#include "common.h"

/* LoadSaveGump's BCC size. */
inline constexpr u16 LOAD_SAVE_GUMP_SIZE = 0x3c;
/* The gump's backdrop is the picture the constructor reads (backdrop), and
 * its slots are framed over it, the selected one in SELECTED_SLOT_COLOUR,
 * the others in FDh (No Regret's); No Remorse's draws four gump shapes and the slot's shape, and frames the selected
 * one alone (paintSelf). */
inline constexpr bool LOAD_SAVE_PICTURE = false;
inline constexpr u8 SELECTED_SLOT_COLOUR = 0xfd;
/* The first slot's place from the backdrop's corner, and the pointer
 * (LoadSaveGump). */
inline constexpr i16 SLOTS_X = 72;
inline constexpr i16 SLOTS_Y = 43;
inline constexpr u16 LOAD_SAVE_POINTER = 0x25;
#endif
