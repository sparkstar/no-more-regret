/* 12d0:04a0-0512 (random.asm): seeded pseudo-random numbers. */
#ifndef PORT_RANDOM_H
#define PORT_RANDOM_H
#include "common.h"

extern "C" {
/* Steps the seed and returns its high word. */
u16 randomNext(void);
/* A random number from low to high, both included. */
u16 randomRange(u16 low, u16 high);
/* The seed is kept odd. */
void setRandomSeed(u32 seed);
u32 getRandomSeed(void);
/* A random number below limit; 0 when limit is 0. */
u16 randomBelow(u16 limit);
}
/* randomBelow for C++ callers; its argument goes through a frame temporary
 * (randomBetween, 12d8:0293). */
inline u16 randomNumber(u16 limit)
{
    return randomBelow(limit);
}
/* 1 once in limit times. */
inline bool oneIn(u16 limit)
{
    return randomBelow(limit) == 0;
}
#endif
