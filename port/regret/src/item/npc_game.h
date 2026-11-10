/* No Regret's values of the NPCs (common/src/item/npc.cpp), which npc.h
 * includes. */
#ifndef PORT_NPC_GAME_H
#define PORT_NPC_GAME_H
#include "animprim.h"

/* The equipment slots: weapons 1-12, then 13-20 and 21-44. */
inline constexpr int EQUIPMENT_SLOTS = 0x2d;
/* The carried equipment's room: EQUIPMENT_SLOTS here; the one program's build takes the larger of
 * the two games'. */
inline constexpr int EQUIPMENT_SLOTS_CAPACITY = EQUIPMENT_SLOTS;
/* The bytes of an NpcData (the saves' and the original's). */
inline constexpr int NPC_DATA_SIZE = 0x97;
/* The ranges weaponSlot, utilitySlot and packSlot look in. */
inline constexpr SlotRange WEAPON_SLOTS = {1, 12};
inline constexpr SlotRange UTILITY_SLOTS = {13, 20};
inline constexpr SlotRange PACK_SLOTS = {21, 44};
/* The base's map (onBaseMap). */
inline constexpr u16 BASE_MAP = 25;
/* The NPCs whose numbers allocate never takes for a new NPC
 * (none). */
inline constexpr std::array<u16, 0> KEPT_NPC_SHAPES{};
/* The NPCs create makes female. */
inline constexpr std::array<u16, 1> FEMALE_SHAPES = {0x597};
/* A teleport compacts the memory (Npc::teleport). */
inline constexpr bool TELEPORT_COMPACTS_MEMORY = true;
/* A teleport starts the tracker again (Npc::teleport): No Regret's loadMap
 * does. */
inline constexpr bool TELEPORT_RESTARTS_TRACKER = false;
/* An NPC in combat may leave a feint attack for another activity
 * (Npc::setActivity). */
inline constexpr bool FEINTING_NPC_MAY_CHANGE = true;
/* A new activity puts the NPC's weapon away first (Npc::setActivity). */
inline constexpr bool NEW_ACTIVITY_PUTS_WEAPON_AWAY = true;
/* Activity 0Dh is the rolling thunder (Npc::setActivity). */
inline constexpr bool ROLLING_THUNDER = true;
/* Npc::doAnim (each game's): the animations the Silencer
 * kneels in, to -1; animation 16h and the kneeling slide right, which No
 * Remorse's halts on; the forward roll gives the reticle back and makes the
 * controlled NPC dodge, else it keeps the reticle; the kneeling moves give
 * the reticle back; the combat runs make the controlled NPC dodge; the NPCs
 * besides the controlled one whose animations take the reticle. */
inline constexpr std::array<i16, 16> KNEELING_ANIMS = {
    ANIM_KNEEL_START,      ANIM_KNEELING_SMALL,      ANIM_KNEELING_LARGE,       ANIM_KNEEL_FIRE_SMALL,
    ANIM_KNEEL_FIRE_LARGE, ANIM_COMBAT_ROLL_LEFT,    ANIM_COMBAT_ROLL_RIGHT,    ANIM_KNEEL_ROLL_LEFT,
    ANIM_KNEEL_ROLL_RIGHT, ANIM_BRIGHT_KNEEL_FIRE,   ANIM_ROLL_FORWARD,         ANIM_KNEELING_RETREAT,
    ANIM_KNEELING_ADVANCE, ANIM_KNEELING_SLIDE_LEFT, ANIM_KNEELING_SLIDE_RIGHT, -1};
inline constexpr bool ANIM_16_AND_SLIDE_RIGHT = true;
inline constexpr bool ROLL_FORWARD_DODGES = true;
inline constexpr bool KNEELING_MOVES_HAND_BACK = true;
inline constexpr bool COMBAT_RUN_DODGES = true;
inline constexpr std::array<u16, 0> RETICLE_NPC_SHAPES = {};
#endif
