/* No Regret's declarations of the explosions' module (common/src/game/
 * explode.h): what differs between the games. */
#ifndef PORT_EXPLODE_GAME_H
#define PORT_EXPLODE_GAME_H
#include <array>
#include "common.h"

/* An explosion alerts the NPCs (unless they were alerted lately), and its
 * sound comes from the exploding item (explode). */
inline constexpr bool EXPLOSION_ALERTS = true;
inline constexpr bool EXPLOSION_SOUND_AT_ITEM = true;
/* fireExplosion's blasts: the grenades' (fire types 3 and 4, one at random)
 * and the rockets' (10, one of the first three; No Regret's boss's grenade,
 * 16h, the fourth). */
inline constexpr std::array<u16, 4> GRENADE_BLASTS = {0x326, 0x320, 0x321, 0x323};
inline constexpr std::array<u16, 4> ROCKET_BLASTS = {0x31c, 0x31f, 0x321, 0x31f};
/* fireExplosion's sounds of fire types 5, 6 and 0Eh, and the bullets' at
 * one chance in 3 for type 2 and one in 4 for 0Dh. */
inline constexpr bool MORE_HIT_SOUNDS = true;
#endif
