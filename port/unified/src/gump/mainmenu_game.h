/* Written by port/tools/make_unified_values.py from regret/src/gump/mainmenu_game.h and
 * remorse/src/gump/mainmenu_game.h: the one program's run-time values; do not edit. */

#ifndef PORT_MAINMENU_GAME_H
#define PORT_MAINMENU_GAME_H
#include <span>
#include <array>
#include "common.h"

extern i16 MAIN_MENU_HEIGHT;
extern bool MAIN_MENU_PLAYS_SOUND;
extern u16 MAIN_MENU_SOUND;
extern bool MAIN_MENU_ALT_X_QUITS;
extern u16 DIFFICULTY_BUTTON_SIZE;
extern u8 DIFFICULTY_CHOSEN_COLOUR;
extern bool DIFFICULTY_FRAMES_UNCHOSEN;
extern u8 DIFFICULTY_UNCHOSEN_COLOUR;
extern bool DIFFICULTY_BUTTON_SOUNDS;
extern std::array<i16, 4> DIFFICULTY_SOUNDS;
extern bool OPTIONS_IN_MENU_FRAME;
extern u8 OPTIONS_FOCUS_COLOUR;
extern bool BLASTS_LIT_AT_ONE;
#endif
