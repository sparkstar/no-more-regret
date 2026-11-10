/* No Regret's values of the load and save gumps (common/src/gump/loadsave.h). */
#ifndef PORT_LOADSAVE_GAME_H
#define PORT_LOADSAVE_GAME_H
#include "common.h"

/* LoadSaveGump's BCC size. */
inline constexpr u16 LOAD_SAVE_GUMP_SIZE = 0x40;
/* The gump's backdrop is the picture the constructor reads (backdrop), and
 * its slots are framed over it, the selected one in SELECTED_SLOT_COLOUR,
 * the others in FDh: No Regret's (paintSelf). */
inline constexpr bool LOAD_SAVE_PICTURE = true;
inline constexpr u8 SELECTED_SLOT_COLOUR = 0x6f;
/* The first slot's place from the backdrop's corner, and the pointer
 * (LoadSaveGump). */
inline constexpr i16 SLOTS_X = 53;
inline constexpr i16 SLOTS_Y = 44;
inline constexpr u16 LOAD_SAVE_POINTER = 0x28;
#endif
