/* No Regret's values of the port's settings gump (common/src/gump/settings.cpp). */
#ifndef PORT_SETTINGS_GAME_H
#define PORT_SETTINGS_GAME_H
#include "common.h"
#include "rect.h"

/* The room inside the red frame of the main menu's picture (gumps.flx
 * 0x36-0x39, MAIN_MENU_HEIGHT high), which the settings take. */
inline constexpr Rect SETTINGS_AREA = Rect(10, 94, 627, 385);
/* The room between the settings' rows. */
inline constexpr s16 SETTINGS_ROW_GAP = 8;

/* The sound the options gump opens with (its constructor, 13b8:0f53). */
inline constexpr bool SETTINGS_OPEN_PLAYS_SOUND = true;
inline constexpr u16 SETTINGS_OPEN_SOUND = 0x1a2;
#endif
