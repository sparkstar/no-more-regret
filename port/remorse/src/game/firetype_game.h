/* No Remorse's fire types (common/src/game/firetype.cpp), which firetype.h
 * includes. */
#ifndef PORT_FIRETYPE_GAME_H
#define PORT_FIRETYPE_GAME_H

/* The table's types. */
inline constexpr int FIRE_TYPES = 0x10;
/* The table (No Regret's 1480:2b2e, No Remorse's 1478:252a;
 * firetype_game.cpp). */
extern FireType (&fireTypes)[FIRE_TYPES];
#endif
