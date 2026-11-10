/* Written by port/tools/make_unified_values.py from regret/src/game/control_game.h and remorse/src/game/control_game.h:
 * the one program's run-time values; do not edit. */

#ifndef PORT_CONTROL_GAME_H
#define PORT_CONTROL_GAME_H
#include <span>
#include <array>
#include "common.h"
#include "animprim.h"
#include "dtable.h"

extern u16 AUTO_FIRER_RATE;
extern u16 CHEAT_ITEM_QUALITY;
extern std::array<u16, 5> WATCHFUL_SHAPES;
extern std::span<const u16> NON_COMBAT_ANIMS;
extern bool AUTO_FIRER_USES_AMMO;
extern bool RETICLE_HIDDEN_WHILE_HELD;
extern i16 FIRST_RETICLE_STYLE;
extern int CONTROLLER_BLOCK;
inline constexpr int CONTROLLER_BLOCK_CAPACITY = 79;
extern bool BUSY_NPC_TURNS;
extern bool ROLL_FORWARD_STEPS;
extern bool AUTO_FIRE_STOPS_EMPTY;
extern std::span<const u16> UNARMED_SHOOTERS;
extern bool RP_32_NEVER_DRY;
extern std::array<i16, 13> NUMBER_KEY_WEAPONS;
extern std::array<i16, 6> WEAPON_LIST_1;
extern std::array<i16, 6> WEAPON_LIST_2;
extern std::array<i16, 4> WEAPON_LIST_3;
extern bool WEAPON_LISTS;
extern bool NUMBER_KEYS_SELECT_WEAPONS;
extern std::span<const CheatGift> CHEAT_GIFTS;
extern std::span<const u16> CHEAT_CLIPS;
extern bool CHEAT_GIVES_CREDITS;
extern u32 CHEAT_CREDITS;
extern bool CHEAT_IMMORTALITY_ALONE;
extern bool DEBUG_KEY_FLUSHES;
extern bool SHAPE_5D6_IS_DEAD;
extern bool RETICLE_STYLE_KEY;
extern bool RETURN_KEY_ENDS_STASIS;
extern bool JUMP_UNREADIES_WEAPON;
extern bool FIRE_INTERRUPTS_BUSY;
extern bool JASSICA_JOKE;
extern bool MORE_CHEAT_CODES;
extern bool MOVE_NO_REGRET;
extern i16 BOSS_SHIELD_START;
#endif
