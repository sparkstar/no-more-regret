/* No Regret's values of the status display (common/src/gump/status.cpp). */
#ifndef PORT_STATUS_GAME_H
#define PORT_STATUS_GAME_H
#include "common.h"

/* The parts' backgrounds are drawn as they are (paint mode 0) (initStatus). */
inline constexpr u8 STATUS_PAINT_MODE = 0;
/* The status display's layout (drawStatus): the weapon's and the item's names
 * (with the data link's MESSAGE and an active item's ACTIVE) and the item's
 * count by the right edge, the ammunition's font and places, and the
 * health and energy bars' length (of full health), place and height; No
 * Regret's bar leaves out shape 5D6h. */
inline constexpr bool STATUS_NAMES = true;
inline constexpr i16 STATUS_AMMO_FONT = 8;
inline constexpr i16 STATUS_AMMO_X = 38;
inline constexpr i16 STATUS_CLIPS_X = 90;
inline constexpr i16 STATUS_AMMO_Y = 8;
inline constexpr u16 STATUS_BAR_LENGTH = 64;
inline constexpr i16 STATUS_BAR_X = 35;
inline constexpr i16 STATUS_BAR_Y = 8;
inline constexpr i16 STATUS_BAR_HEIGHT = 11;
inline constexpr bool STATUS_NO_HEALTH_SHAPE = true;
#endif
