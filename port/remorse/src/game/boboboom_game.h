/* No Remorse's values of the NPCs' sounds (common/src/game/boboboom.cpp). */
#ifndef PORT_BOBOBOOM_GAME_H
#define PORT_BOBOBOOM_GAME_H
#include <array>
#include "common.h"

/* The cries of men and women of shape 576h and 596h with the violence on, to -1
 * (makeNpcSound). */
inline constexpr std::array<i16, 4> MALE_CRIES = {0x65, 0x66, 0x67, -1};
inline constexpr std::array<i16, 3> FEMALE_CRIES = {0xb, 0xa, -1};
/* Shapes 5B1h (the boss's boomer), 58Fh, 59Ch, 5D6h, 62Dh, 596h, 278h and
 * 656h have sounds of their own (makeNpcSound): No Regret's. */
inline constexpr bool SHAPE_SOUNDS = false;
#endif
