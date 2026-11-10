/* No Remorse's values of the main menu (common/src/gump/mainmenu.cpp). */
#ifndef PORT_MAINMENU_GAME_H
#define PORT_MAINMENU_GAME_H
#include <array>
#include "common.h"

/* The main menu's height: its four quarters' shapes, two of them high
 * (MainMenuGump::paintSelf). */
inline constexpr i16 MAIN_MENU_HEIGHT = 240;
/* The main menu plays a sound when it opens and when it comes back from the
 * settings (MainMenuGump, choose): not No Remorse's. */
inline constexpr bool MAIN_MENU_PLAYS_SOUND = false;
inline constexpr u16 MAIN_MENU_SOUND = 0;
/* Alt+X quits from the main menu (MainMenuGump::handleKeyboard): not in No
 * Remorse. */
inline constexpr bool MAIN_MENU_ALT_X_QUITS = false;
/* DifficultyButton's BCC size. */
inline constexpr u16 DIFFICULTY_BUTTON_SIZE = 0x3e;
/* The difficulty dialog's chosen level is framed in this colour; No Remorse frames no other (No Regret's in EEh)
 * (DifficultyButton::paintSelf). */
inline constexpr u8 DIFFICULTY_CHOSEN_COLOUR = 0x44;
inline constexpr bool DIFFICULTY_FRAMES_UNCHOSEN = false;
inline constexpr u8 DIFFICULTY_UNCHOSEN_COLOUR = 0;
/* A difficulty button plays its sound when chosen (DifficultyButton::showPressed): not No Remorse's. */
inline constexpr bool DIFFICULTY_BUTTON_SOUNDS = false;
/* The difficulty buttons' sounds, levels 1-4 (DifficultyGump): none in No Remorse (DIFFICULTY_BUTTON_SOUNDS). */
inline constexpr std::array<i16, 4> DIFFICULTY_SOUNDS = {0, 0, 0, 0};
/* The options gump sits in the main menu's frame: No Regret's. */
inline constexpr bool OPTIONS_IN_MENU_FRAME = false;
/* The colour of the options gump's focus frame. */
inline constexpr u8 OPTIONS_FOCUS_COLOUR = 0xc;
/* The options gump's LIMITED is lit when limitBlasts is 1: No Regret's; No
 * Remorse's when it is not 0. */
inline constexpr bool BLASTS_LIT_AT_ONE = false;
#endif
