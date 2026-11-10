/* No Regret's values of the falls (common/src/item/gravity.cpp). */
#ifndef PORT_GRAVITY_GAME_H
#define PORT_GRAVITY_GAME_H

/* The controlled NPC's fall starts at once; another item's after 20 ticks
 * (hurl). */
inline constexpr bool CONTROLLED_FALL_AT_ONCE = true;
/* GravityProcess's BCC size. */
inline constexpr u16 GRAVITY_PROCESS_SIZE = 0x43;
/* The falling process's first run ends a fixed item's fall, and a weightless
 * one's unless it is a crate (GravityProcess::run). */
inline constexpr bool FALL_CHECKED_FIRST = true;
#endif
