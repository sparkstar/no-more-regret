/* Written by port/tools/make_unified_values.py from regret/src/world/world_game.h and remorse/src/world/world_game.h:
 * the one program's run-time values; do not edit. */

#ifndef PORT_WORLD_GAME_H
#define PORT_WORLD_GAME_H
#include <span>
#include <array>
#include "common.h"
#include "dtable.h"
#include "rect.h"

extern std::span<const u16> UNSTACKED_SHAPES;
extern u16 SELECT_SOUND;
extern bool SELECT_SOUND_WAITS;
extern bool CREDITS_END_TIDILY;
extern bool THAW_REDRAWS_SPRITES;
extern bool THAW_ENDS_TELEPORT;
extern bool INTRINSICS_SHOP;
extern const char *INTRINSICS_TEXT;
extern bool HELP_PAGE_SOUNDS;
extern bool WEAPON_KEYS_SILENCER_ONLY;
extern bool PREVIOUS_WEAPON_SETS_STANCE;
extern std::array<const char *, 5> HELP_FILES;
extern i16 HELP_PAGES;
extern bool HELP_PAGES_WRAP;
extern bool HELP_BUTTONS;
extern u16 HELP_SOUND;
extern bool HELP_ARROW_KEYS;
extern u16 QUIT_SOUND;
extern u16 QUICK_LOAD_SOUND;
extern u16 QUICK_SAVE_SOUND;
extern u16 VERSION_SOUND;
extern Point QUESTION_YES;
extern Point QUESTION_NO;
extern Point NOTICE_PLACE;
extern std::span<const u16> DETONATED_SHAPES;
extern std::span<const ItemKey> ITEM_KEYS;
extern std::span<const u16> UNCHARGED_ITEMS;
extern bool ITEM_USED_FIRST;
extern i16 VERSION_FRAGMENTS;
extern u16 KEPT_WEAPON;
extern bool TARGETING_KEY;
extern bool POSITION_KEY;
extern bool MORE_DEBUG_KEYS;
extern bool WORLD_KEYS_NO_REGRET;
#endif
