/* No Remorse's values of the falls (common/src/item/gravity.cpp). */
#ifndef PORT_GRAVITY_GAME_H
#define PORT_GRAVITY_GAME_H

/* The controlled NPC's fall starts at once (No Regret's): No Remorse's after
 * 20 ticks, as any item's (hurl). */
inline constexpr bool CONTROLLED_FALL_AT_ONCE = false;
/* GravityProcess's BCC size. */
inline constexpr u16 GRAVITY_PROCESS_SIZE = 0x42;
/* The falling process's first run ends a fixed item's fall, and a weightless
 * one's unless it is a crate (GravityProcess::run): No Regret's. */
inline constexpr bool FALL_CHECKED_FIRST = false;
#endif
