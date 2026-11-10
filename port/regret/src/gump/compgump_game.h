/* No Regret's waits of the computer's text (common/src/gump/compgump.cpp):
 * a character's, Borland's delay of 30 milliseconds, and after a '*', of
 * 1800 (under the kernel's faster timer a part of them: 3-8 ms and 0.37 s
 * with the sound on). */
#ifndef PORT_COMPGUMP_GAME_H
#define PORT_COMPGUMP_GAME_H
#include "common.h"

/* A character waits for the system timer's next tick: No Remorse's; else
 * Borland's delay of CHARACTER_WAIT milliseconds. */
inline constexpr bool CHARACTER_WAITS_A_TICK = false;
inline constexpr u16 CHARACTER_WAIT = 30;
/* The wait after a '*', Borland's delay in milliseconds. */
inline constexpr u16 PAUSE_WAIT = 1800;
#endif
