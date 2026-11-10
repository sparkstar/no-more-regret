/* No Remorse's values of the port's settings gump (common/src/gump/settings.cpp). */
#ifndef PORT_SETTINGS_GAME_H
#define PORT_SETTINGS_GAME_H
#include "common.h"
#include "rect.h"

/* The room inside the red frame of the main menu's picture (gumps.flx
 * 0x36-0x39, MAIN_MENU_HEIGHT high), which the settings take. */
inline constexpr Rect SETTINGS_AREA = Rect(11, 132, 628, 349);
/* The room between the settings' rows: the area is lower than No Regret's,
 * and the left column's seven rows fit it with less. */
inline constexpr s16 SETTINGS_ROW_GAP = 5;

/* The options gump opens with a sound: No Regret's. */
inline constexpr bool SETTINGS_OPEN_PLAYS_SOUND = false;
inline constexpr u16 SETTINGS_OPEN_SOUND = 0;
#endif
