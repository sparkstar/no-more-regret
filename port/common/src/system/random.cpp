/* SYSTEM\RANDOM, random.asm (12d0:04a0-0512) in C++. */
#include "random.h"
#include "gamestate.h"

namespace {
u32 &randomSeed = gameGlobal<u32>(0x0bad0bad);
}

/* seed * 3 + (seed << 16) + 1 as the original's carry chain adds it: INC
 * adds the 1 to the low word without a carry into the high word, and the
 * ADC after it adds the carry out of the high word back into it. */
u16 randomNext()
{
    const u32 triple = randomSeed * 3;
    const u16 low = static_cast<u16>(triple + 1);
    u32 high = (triple >> 16) + (randomSeed & 0xffff);
    high += high >> 16;
    randomSeed = static_cast<u32>(static_cast<u16>(high)) << 16 | low;
    return static_cast<u16>(high);
}

/* The span wraps to 0 for 0..FFFFh, and the original then skips the
 * division and returns low, not a random number. */
u16 randomRange(u16 low, u16 high)
{
    const u16 number = randomNext();
    if (high < low) {
        const u16 swap = low;
        low = high;
        high = swap;
    }
    const u16 span = static_cast<u16>(high - low + 1);
    if (span == 0) {
        return low;
    }
    return static_cast<u16>(low + (number % span));
}

void setRandomSeed(u32 seed)
{
    randomSeed = seed | 1;
}

u32 getRandomSeed()
{
    return randomSeed;
}

u16 randomBelow(u16 limit)
{
    const u16 number = randomNext();
    return (limit != 0U) ? static_cast<u16>(number % limit) : 0;
}
