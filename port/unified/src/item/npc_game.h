/* Written by port/tools/make_unified_values.py from regret/src/item/npc_game.h and remorse/src/item/npc_game.h: the one
 * program's run-time values; do not edit. */

#ifndef PORT_NPC_GAME_H
#define PORT_NPC_GAME_H
#include <span>
#include "animprim.h"

extern int EQUIPMENT_SLOTS;
inline constexpr int EQUIPMENT_SLOTS_CAPACITY = 45;
extern int NPC_DATA_SIZE;
extern SlotRange WEAPON_SLOTS;
extern SlotRange UTILITY_SLOTS;
extern SlotRange PACK_SLOTS;
extern u16 BASE_MAP;
extern std::span<const u16> KEPT_NPC_SHAPES;
extern std::span<const u16> FEMALE_SHAPES;
extern bool TELEPORT_COMPACTS_MEMORY;
extern bool TELEPORT_RESTARTS_TRACKER;
extern bool FEINTING_NPC_MAY_CHANGE;
extern bool NEW_ACTIVITY_PUTS_WEAPON_AWAY;
extern bool ROLLING_THUNDER;
extern std::span<const i16> KNEELING_ANIMS;
extern bool ANIM_16_AND_SLIDE_RIGHT;
extern bool ROLL_FORWARD_DODGES;
extern bool KNEELING_MOVES_HAND_BACK;
extern bool COMBAT_RUN_DODGES;
extern std::span<const u16> RETICLE_NPC_SHAPES;
#endif
