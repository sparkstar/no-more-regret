/* No Remorse's waits of the computer's text
 * (common/src/gump/compgump.cpp): a character's, the system timer's next
 * tick (1398:09f2), and after a '*', Borland's delay of 4800 milliseconds
 * (1398:0974; about 1 s with the sound on). */
#ifndef PORT_COMPGUMP_GAME_H
#define PORT_COMPGUMP_GAME_H
#include "common.h"

/* A character waits for the system timer's next tick; No Regret's, Borland's
 * delay of CHARACTER_WAIT milliseconds. */
inline constexpr bool CHARACTER_WAITS_A_TICK = true;
inline constexpr u16 CHARACTER_WAIT = 0;
/* The wait after a '*', Borland's delay in milliseconds. */
inline constexpr u16 PAUSE_WAIT = 4800;
#endif
