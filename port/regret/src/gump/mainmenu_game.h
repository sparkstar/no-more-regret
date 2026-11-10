/* No Regret's values of the main menu (common/src/gump/mainmenu.cpp). */
#ifndef PORT_MAINMENU_GAME_H
#define PORT_MAINMENU_GAME_H
#include <array>
#include "common.h"

/* The main menu's height: its four quarters' shapes, two of them high
 * (MainMenuGump::paintSelf). */
inline constexpr i16 MAIN_MENU_HEIGHT = 330;
/* The main menu plays a sound when it opens and when it comes back from the
 * settings (MainMenuGump, choose). */
inline constexpr bool MAIN_MENU_PLAYS_SOUND = true;
inline constexpr u16 MAIN_MENU_SOUND = 0x1a1;
/* Alt+X quits from the main menu (MainMenuGump::handleKeyboard). */
inline constexpr bool MAIN_MENU_ALT_X_QUITS = true;
/* DifficultyButton's BCC size. */
inline constexpr u16 DIFFICULTY_BUTTON_SIZE = 0x40;
/* The difficulty dialog's chosen level is framed in this colour; the others in DIFFICULTY_UNCHOSEN_COLOUR
 * (DifficultyButton::paintSelf). */
inline constexpr u8 DIFFICULTY_CHOSEN_COLOUR = 0xf1;
inline constexpr bool DIFFICULTY_FRAMES_UNCHOSEN = true;
inline constexpr u8 DIFFICULTY_UNCHOSEN_COLOUR = 0xee;
/* A difficulty button plays its sound when chosen (DifficultyButton::showPressed). */
inline constexpr bool DIFFICULTY_BUTTON_SOUNDS = true;
/* The difficulty buttons' sounds, levels 1-4 (DifficultyGump). */
inline constexpr std::array<i16, 4> DIFFICULTY_SOUNDS = {0x19d, 0x19e, 0x19f, 0x1a0};
/* The options gump sits in the main menu's frame: No Regret's. */
inline constexpr bool OPTIONS_IN_MENU_FRAME = true;
/* The colour of the options gump's focus frame. */
inline constexpr u8 OPTIONS_FOCUS_COLOUR = 0x74;
/* The options gump's LIMITED is lit when limitBlasts is 1: No Regret's; No
 * Remorse's when it is not 0. */
inline constexpr bool BLASTS_LIT_AT_ONE = true;
#endif
