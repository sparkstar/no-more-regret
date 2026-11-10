/* No Remorse's values of the status display (common/src/gump/status.cpp). */
#ifndef PORT_STATUS_GAME_H
#define PORT_STATUS_GAME_H
#include "common.h"

/* The parts' backgrounds are drawn through the remap table (paint mode 1) (initStatus). */
inline constexpr u8 STATUS_PAINT_MODE = 1;
/* The status display's layout (drawStatus): No Regret's the weapon's and the item's names
 * (with the data link's MESSAGE and an active item's ACTIVE) and the item's
 * count by the right edge (No Remorse's count at 75), the ammunition's font and places, and the
 * health and energy bars' length (of full health), place and height; No
 * Regret's bar leaves out shape 5D6h. */
inline constexpr bool STATUS_NAMES = false;
inline constexpr i16 STATUS_AMMO_FONT = 15;
inline constexpr i16 STATUS_AMMO_X = 30;
inline constexpr i16 STATUS_CLIPS_X = 75;
inline constexpr i16 STATUS_AMMO_Y = 11;
inline constexpr u16 STATUS_BAR_LENGTH = 66;
inline constexpr i16 STATUS_BAR_X = 34;
inline constexpr i16 STATUS_BAR_Y = 7;
inline constexpr i16 STATUS_BAR_HEIGHT = 13;
inline constexpr bool STATUS_NO_HEALTH_SHAPE = false;
#endif
