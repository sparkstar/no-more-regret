/* No Remorse's declarations of the explosions' module (common/src/game/
 * explode.h): what differs between the games. */
#ifndef PORT_EXPLODE_GAME_H
#define PORT_EXPLODE_GAME_H
#include <array>
#include "common.h"

/* An explosion alerts the NPCs (unless they were alerted lately), and its sound comes from the exploding item;
 * No Remorse's does not alert, and its sound comes from the controlled NPC (kept) (explode). */
inline constexpr bool EXPLOSION_ALERTS = false;
inline constexpr bool EXPLOSION_SOUND_AT_ITEM = false;
/* fireExplosion's blasts: the grenades' (fire types 3 and 4, one at random)
 * and the rockets' (10, one of the first three; No Regret's boss's grenade,
 * 16h, the fourth). */
inline constexpr std::array<u16, 3> GRENADE_BLASTS = {0x326, 0x320, 0x321};
inline constexpr std::array<u16, 3> ROCKET_BLASTS = {0x31c, 0x31f, 0x322};
/* fireExplosion's sounds of fire types 5, 6 and 0Eh, and the bullets' at
 * one chance in 3 for type 2 and one in 4 for 0Dh: No Regret's; No Remorse's types 5, 6, 0Eh and 2 make none and 0Dh
 * always one. */
inline constexpr bool MORE_HIT_SOUNDS = false;
#endif
