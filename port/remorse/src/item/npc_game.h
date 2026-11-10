/* No Remorse's values of the NPCs (common/src/item/npc.cpp), which npc.h
 * includes. */
#ifndef PORT_NPC_GAME_H
#define PORT_NPC_GAME_H
#include "animprim.h"

/* The equipment slots: weapons 1-5, ammunition and utilities 6-10, the pack 11-25; every NpcData member after
 * equipment sits 30 bytes lower than in No Regret's 45 (mana, attackTarget, hitTime). */
inline constexpr int EQUIPMENT_SLOTS = 0x1e;
/* The carried equipment's room: EQUIPMENT_SLOTS here; the one program's build takes the larger of
 * the two games'. */
inline constexpr int EQUIPMENT_SLOTS_CAPACITY = EQUIPMENT_SLOTS;
/* The bytes of an NpcData (the saves' and the original's). */
inline constexpr int NPC_DATA_SIZE = 0x79;
/* The ranges weaponSlot, utilitySlot and packSlot look in. */
inline constexpr SlotRange WEAPON_SLOTS = {1, 5};
inline constexpr SlotRange UTILITY_SLOTS = {6, 10};
inline constexpr SlotRange PACK_SLOTS = {11, 25};
/* The base's map (onBaseMap). */
inline constexpr u16 BASE_MAP = 40;
/* The NPCs whose numbers allocate never takes for a new NPC
 * (No Remorse's own). */
inline constexpr std::array<u16, 5> KEPT_NPC_SHAPES = {0x3ac, 0x4dc, 0x524, 0x4d9, 0x4e0};
/* The NPCs create makes female. */
inline constexpr std::array<u16, 2> FEMALE_SHAPES = {0x597, 0x3ac};
/* A teleport compacts the memory (Npc::teleport): not No Remorse's. */
inline constexpr bool TELEPORT_COMPACTS_MEMORY = false;
/* A teleport starts the tracker again (Npc::teleport), not loadMap as No
 * Regret's. */
inline constexpr bool TELEPORT_RESTARTS_TRACKER = true;
/* An NPC in combat may leave a feint attack for another activity (Npc::setActivity):
 * No Regret's (kept). */
inline constexpr bool FEINTING_NPC_MAY_CHANGE = false;
/* A new activity puts the NPC's weapon away first (Npc::setActivity): No Regret's (No Remorse's only for a standing
 * attack, kept). */
inline constexpr bool NEW_ACTIVITY_PUTS_WEAPON_AWAY = false;
/* Activity 0Dh is the rolling thunder (Npc::setActivity): No Regret's; No Remorse's NPC stands. */
inline constexpr bool ROLLING_THUNDER = false;
/* Npc::doAnim (each game's): the animations the Silencer
 * kneels in, to -1; animation 16h and the kneeling slide right, which No
 * Remorse's halts on; the forward roll gives the reticle back and makes the
 * controlled NPC dodge, else it keeps the reticle; the kneeling moves give
 * the reticle back; the combat runs make the controlled NPC dodge; the NPCs
 * besides the controlled one whose animations take the reticle. */
inline constexpr std::array<i16, 11> KNEELING_ANIMS = {ANIM_KNEEL_START,
                                                       ANIM_KNEELING_SMALL,
                                                       ANIM_KNEELING_LARGE,
                                                       ANIM_KNEEL_FIRE_SMALL,
                                                       ANIM_KNEEL_FIRE_LARGE,
                                                       ANIM_COMBAT_ROLL_LEFT,
                                                       ANIM_COMBAT_ROLL_RIGHT,
                                                       ANIM_KNEEL_ROLL_LEFT,
                                                       ANIM_KNEEL_ROLL_RIGHT,
                                                       ANIM_BRIGHT_KNEEL_FIRE,
                                                       -1};
inline constexpr bool ANIM_16_AND_SLIDE_RIGHT = false;
inline constexpr bool ROLL_FORWARD_DODGES = false;
inline constexpr bool KNEELING_MOVES_HAND_BACK = false;
inline constexpr bool COMBAT_RUN_DODGES = false;
inline constexpr std::array<u16, 1> RETICLE_NPC_SHAPES = {0x580};
#endif
