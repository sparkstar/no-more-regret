/* CPROTMEM.C's sharing of the machine's memory
 * (port/common/src/system/cprotmem.cpp): the fragments the original makes on
 * its machine of 16 MB (DOSBox, memsize=16, REGRET -x25: "System memory:
 * 15014912 bytes", fragment 0 11260928 bytes, 1 315000, the largest block
 * 11260886), and the room the caches' blocks take as the original's heap
 * gives it. */
#include "cprotmem.h"
#include "gamestate.h"
#include "system.h"

#include <cstdio>
#include <cstdlib>
#include <string>

/* What cprotmem.cpp calls of the rest of the game, which the test does not
 * make: the compaction's purge (never, the blocks fit). */
System *&currentSystem = gameGlobal<System *>(nullptr);

void halt(std::source_location place)
{
    std::printf("halt %s %u\n", place.file_name(), (unsigned)place.line());
    std::exit(2);
}

namespace {

int checks = 0, failures = 0;

void check(bool ok, const std::string &what)
{
    checks++;
    if (!ok) {
        failures++;
        std::printf("MISMATCH cprotmem: %s\n", what.c_str());
    }
}

u32 largestOf(u16 fragment)
{
    u32 total, largest;
    fragmentFreeSpace(fragment, &total, &largest);
    return largest;
}

}

int main()
{
    check(systemMemory == 15014912, "the machine's memory");
    check(firstFragmentPercent == 75, "fragment 0's share of 14 to 16 million bytes");
    check(fragments[0].size == 11260928, "fragment 0, 75% of the kilobytes");
    check(fragments[SOUND_FRAGMENT].size == 315000, "the sound fragment");
    check(fragmentCount == 3 && fragments[2].size == 15014912 - 11260928 - 315000, "the rest in one fragment");
    u32 total, largest;
    u16 selectors;
    memoryStatus(&total, &largest, &selectors);
    check(largest == 11260886, "the largest block as the original reports it");
    /* The sound cache: all of the sound fragment but 19096 bytes, then the
     * rest of its heap. */
    u32 sound = largestOf(SOUND_FRAGMENT);
    check(sound == 314958, "the sound fragment's free space");
    void *soundCache = allocateMemory(sound - 15000 - 4096, 0, SOUND_FRAGMENT);
    check(soundCache && largestOf(SOUND_FRAGMENT) == 19086, "the sound fragment after the sound cache");
    /* The cache: all of fragment 0 but 4K, rounded to pages past 1M. */
    void *cache = allocateMemory(largestOf(0) - 4096, 0, 0);
    check(cache && largestOf(0) == 972, "fragment 0 after the cache");
    void *other = allocateMemory(1000, 0, ANY_FRAGMENT);
    check(other && largestOf(0) == 972 && largestOf(SOUND_FRAGMENT) == 19086, "another block, of the host's heap");
    check(!allocateMemory(0, 0, ANY_FRAGMENT), "0 bytes fail");
    releaseMemory(other, 0);
    releaseMemory(cache, 0);
    releaseMemory(soundCache, 0);
    check(largestOf(0) == 11260886 && largestOf(SOUND_FRAGMENT) == 314958, "the blocks given back");
    std::printf("cprotmem: %d checks, %d mismatches\n", checks, failures);
    return failures ? 1 : 0;
}
